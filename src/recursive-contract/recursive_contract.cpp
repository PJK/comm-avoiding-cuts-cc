/*
 *
 *  Created on: 07 nov 2015
 *      Author: Alessandro De Palma
 */

#include "graph_slice.hpp"
#include "recursive_contract.hpp"
#include <iostream>
#include <chrono>
#include "parallel_contract.hpp"
#include "co_mincut.h"
#include "utils.hpp"
#include "MPICollector.hpp"

#define INFOTAG1 0
#define INFOTAG2 1
#define SLICETAG 2

using namespace std;

namespace mincut {


	/*
	 after this, the root has the smallest cut given by any of the processors
	 */
	long cut_reduce(MPI_Comm comm, long local_cut, int root) {
		long cut_result = local_cut;
		MPI::Reduce(&local_cut, &cut_result, 1, MPI_LONG, MPI_MIN, root, comm); //This overwrites cut with the minimum value just in p0.
		return cut_result;
	}


	/*
	 * Given a starting communicator and a graph, it performs a number of parallelized recursive_contract recursion trees on the graph,
	 * returning the locally found minimum cut (which might not be the real mincut) at all processors except 0, which holds
	   the smallest cut given by any of the processors.
	 *
	 * perform some given number of trials after each other.
	 * every trial is in parallel, but the trials are sequenced after each other
	 * relies on the assumption that V > p and that p = 2^k, for some k (k being a natural number).
	 * When p = 1 a sequential version of the algorithm is called.
	 */
	long parallel_cut(MPI_Comm comm, Graph& graph, int trials, int seed) {
		long cut = numeric_limits<long>::max();
		int rank;

		MPI_Comm_rank(comm, &rank);

		/*if (rank == 0) {
			std::cout << "n trials: " << trials << std::endl;
		}*/

		sitmo::prng_engine random(rank+seed);

		for (int i=0; i<trials; ++i){
			DebugUtils::print(rank, [&](std::ostream & out) {
				out << "working on trial " << i << " out of " << trials;
			});
			//copy graph
			Graph copied_graph = graph.deep_copy();

			DebugUtils::print(rank, [&](std::ostream & out) {
				out << "took deep copy and running RC";
			});

			cut = std::min(cut, parallel_recursive_contract(comm, copied_graph, &random));
		}

		cut = cut_reduce(comm, cut, 0);

		return cut;
	}


	long parallel_cut(MPI_Comm comm, Graph& graph, double success, int seed) {

		int trials = 2*comincut::number_of_trials(graph.get_number_of_vertices(), success);


		return parallel_cut(comm, graph, trials, seed);
	}


	/*
	 * Given a communicator (comm) and the size of the graph, it decides the trial distribution.
	 * localTrials will be the number of trials to execute for the local subset, subsetSize will be the size of each subset.
	 * It relies on the assumption that the max memory on a core is around 2.5GB.
	 */
	void distribute_trials(MPI_Comm comm, int *localTrials, int *subsetSize, int *leftouts, int V, int nTrials) {
		int P, rank;
		int trialsBound, memoryBound;
		int lowerBound;
		int subset; //Temp size.
		int nSubsets; //Number of subsets.
		int trials; //Temp localTrials.
		int missingTrials;
		int localSubset;
		double MAXMEM = 1.0; //Empirically determined, if the max memory on a core is around 2.5GB. Bottleneck is reassign_graph.
		if(V >= 16000) MAXMEM = 1.95; //To let the tests run! This needs the max no. of cores per node set to 16.

		/* After the presentation we might want to do it this way. Nothing changes up to 8k vertices.
		 * To be more consistent maybe put an if (V >= 16000 && P>=32 MAXMEM = 1.95) (let's call it "hand tuning") otherwise we loose some performance on "old" results
		 * and we might want to avoid that.
		double MAXMEM = (V >= 16000) ? 2.6 : 4;
		MAXMEM /= 4; //This is actually true only if the subsetSize is > 1. Otherwise we're being too pessimistic.
		*/

		MPI_Comm_rank(comm, &rank);
		MPI_Comm_size(comm, &P);

		trialsBound = (int) ceil( ((double) P)/(double (nTrials)) ); //The number of groups must be smaller than those of trials.
		memoryBound = (int) ceil( ( pow(V+P, 2)*sizeof(long))/(pow(2, 30)*MAXMEM) ); //One slice must fit into the core memory.

		//DEBUGGING.
		//if(rank == 0) std::cout << "V: " << V << " Mem low bound (processors): " << memoryBound << endl;

		lowerBound = std::max(trialsBound, memoryBound);
		bool flag1 = false;
//    	int counter = 0;

		for(subset = lowerBound; subset <= P; subset++) { //Set the size to lowest possible power of 2.
			for(int check = 1; check <= std::min(P, V); check *= 2) {
				if(subset == check) {
					flag1 = true;
					break;
				}
			}
			if(flag1) break;
		}
		if(!flag1) std::cerr << "Not enough memory for the given graph.";


		*subsetSize = subset;

		*leftouts = P % subset;
		nSubsets = (int) floor( ((double) P)/(double (subset)) );

		trials = (int) floor( ((double) nTrials)/(double (nSubsets)) );
		missingTrials = nTrials - trials*nSubsets;
		localSubset = (int) floor( ((double) rank)/(double (subset)) );
		if(localSubset < missingTrials) trials++;

		*localTrials = trials;

		//DEBUGGING.
		//if(rank == 0) std::cout << "Chosen subset size: " << subset << std::endl;

		return;
	}


	//deprecated: only use for basic testing, as the seed is fixed and not random
	long parallel_recursive_contract(MPI_Comm comm, Graph& graph) {

		int rank;

		MPI_Comm_rank(comm, &rank);

		sitmo::prng_engine random(rank);

		return parallel_recursive_contract(comm, graph, &random);
	}


	/*
	 * Given a starting communicator and a graph, it performs a parallelized recursive_contract recursion tree on the graph,
	 * returning the locally found minimum cut (which might not be the real mincut).
	 * It relies on the assumption that V > p and that p = 2^k, for some k (k being a natural number).
	 * When p = 1 a sequential version of the algorithm is called.
	 *
	 * ANALYSIS: Time = O(log(p)*(O(n^2/p) + O(parallel_contract_procedure)) + O(serial_contract_procedure);
	 */
	long parallel_recursive_contract(MPI_Comm comm, Graph& graph, sitmo::prng_engine * random) {
		int V, p, rank, x, p2;
		long cut;
		MPI_Comm subComm, superComm;

		DebugUtils::print(-1, [&](std::ostream & out) {
			out << "about to touch communicators";
		});

		superComm = comm;
		MPI_Comm_size(superComm, &p);
		MPI_Comm_rank(superComm, &rank);

		assert(p % 2 == 0);

		DebugUtils::print(rank, [&](std::ostream & out) {
			out << "working in group of " << p << " processors";
		});


		V = graph.get_number_of_vertices();
		x = (int) ceil( ((double) V)/sqrt(2.0) + 1.0); //Contract to ceil(n/sqrt(2) + 1).
		p2 = p/2; //p is a power of two.


		//graph.print(superComm);

		/** FIXME: PROFILING? You'll know better **/
		// Yuck :D
		while(p > 1) {

			parallel_contract(superComm, &graph, random, x);

			//DEBUG
			/*int tp;
			int trank;
			MPI_Comm_size(comm, &tp);
			MPI_Comm_rank(comm, &trank);
			for (int i=0; i<tp/p;++i) {
				MPI_Barrier(comm);
				if (i*p <= trank && i*p+p > trank) {
					graph.print(superComm);
				}
			}*/

			assert(graph.get_number_of_vertices() == x);

			reassign_graph(superComm, p2, &graph, x, &subComm);
			MPI_Comm_free(&subComm);

			if(rank < p2) duplicate_graph(superComm, p2, &graph, &subComm);
			else graph = duplicate_graph(superComm, p2, nullptr, &subComm);

			assert(graph.get_number_of_vertices() == x);
			assert(graph.get_size() < x+p2);

			if(superComm != comm) MPI_Comm_free(&superComm);
			superComm = subComm;
			MPI_Comm_size(superComm, &p);
			assert (p == p2);
			MPI_Comm_rank(superComm, &rank);

			V = graph.get_number_of_vertices();
			x = (int) ceil( ((double) V)/sqrt(2.0) + 1.0);
			p2 = p/2;

			//DEBUG
			/*MPI_Comm_size(comm, &tp);
			MPI_Comm_rank(comm, &trank);
			for (int i=0; i<tp/p;++i) {
				MPI_Barrier(comm);
				if (i*p <= trank && i*p+p > trank) {
					graph.print(superComm);
				}
			}*/

		}



		//serial_contract
		assert(graph.get_size() == graph.get_number_of_vertices());

		DebugUtils::print(rank, [&](std::ostream & out) {
			out << "will now run enter the serial part";
		});

		adjacency_matrix<long> matrix(graph.get_number_of_vertices(), graph.begin());

		cut = comincut::minimum_cut_try(&matrix, (*random)());

		graph.free_slice();

		return cut;
	}



	/*
	 * Given a communicator whose first p processors store a distributed graph, copy the graph
	 * distributing it to the remaining commSize - p processors and create two communicators
	 * out of the two mentioned groups. At the end of the call, newComm will contain the new communicator
	 * assigned to the current processor.
	 * The communicators are created now to avoid the creation of an "intercommunicator".
	 * For the time being (p == p2), and the no. of processors is a power of 2.
	 * The destination processes must call this with nullptr as pGraph.
	 *
	 * ANALYSIS: O(1) communications per processor. Sent/received data = O(n^2/p).
	 * 				Computations = O(n^2/p). (copy into/from buffer)
	 */
	/** FIXME: PROFILING? You'll know better **/
// I wonder how expensive this is?
	Graph duplicate_graph(MPI_Comm comm, int p, Graph *pGraph, MPI_Comm *newComm) {

		int w, n, nVertices;
		MPI_Request req1, req2, req3;
		MPI_Datatype type = MPI_LONG;

		int commSize;
		MPI_Comm_size(comm, &commSize);

		int rank; //The rank of the current processor in the communicator.
		MPI_Comm_rank(comm, &rank);

		//int p2 = p - commSize; //Size of the second group of processors.

		if(rank < p) { //Sender processors.

			assert (pGraph != nullptr);

			Graph& graph = *pGraph;

			int dest = p + rank;

			w = graph.get_rows_per_slice();
			n = w*p; //N is the actual dimension of the square matrix.
			nVertices = graph.get_number_of_vertices();

			// Can't this be done as a collective in which all ranks in the new group participate?
			// If this turns out to take significant time, it might be useful -- from what ive seen big collectives
			// are always faster than anything you do by hand
			MPI::Isend(&w, 1, MPI_INT, dest, INFOTAG1, comm, &req1); //Send w to the second group.
			MPI::Isend(&nVertices, 1, MPI_INT, dest, INFOTAG2, comm, &req2); //Send n to the second group.

			MPI::Isend(graph.begin(), w*n, type, dest, SLICETAG, comm, &req3); //Send the slice to the assigned second group processor.

			MPI_Comm_split(comm, 0, 0, newComm);

			MPI::Wait(&req1, MPI_STATUS_IGNORE);
			MPI::Wait(&req2, MPI_STATUS_IGNORE);
			MPI::Wait(&req3, MPI_STATUS_IGNORE);

			return graph;
		}
		else { //Receiver processors.

			int source = rank - p;
			int nElements;

			MPI::Irecv(&w, 1, MPI_INT, source, INFOTAG1, comm, &req1);
			MPI::Irecv(&nVertices, 1, MPI_INT, source, INFOTAG2, comm, &req2);

			MPI::Wait(&req1, MPI_STATUS_IGNORE); //We need w to proceed, now.
			n = w*p;

			long *slice_memory = new long[w*n];
			nElements = n*w;

			MPI::Irecv(slice_memory, nElements, type, source, SLICETAG, comm, &req3);
			MPI::Wait(&req3, MPI_STATUS_IGNORE);

			MPI::Wait(&req2, MPI_STATUS_IGNORE); //We need nVertices to proceed, now.
			Graph copiedGraph (nVertices, w, (rank - p), n, slice_memory);

			MPI_Comm_split(comm, 1, 0, newComm);

			return copiedGraph;
		}
	}

	/*
	 * Given a graph distributed amongst the processes in the communicator, cleans the graph
	 * of the contracted vertices (only x vertices remain) and distributes it on a p2-processors subset of the communicator.
	 * It relies on the only assumption that w2 > w. It can be proved that it always holds for our usage.
	 * At the end of the call pGraph will point to the new one, for the first p2 processes.
	 * newComm is a new communicator to the group of the first p2 processes.
	 *
	 * IMPORTANT: for this method to work the MPI self btl has to be enabled! e.g. "mpirun --mca btl tcp,self".
	 *
	 * ANALYSIS: at most O(p/n) communications per processor: if n > p it's O(1).
	 * 			 Amount of sent/received data is O(n^2/p), the same holds for computations (copy into/from buffer).
	 * 			 Temporary memory occupation might be as much as 2*old_slice_size + 2*new_slice_size.
	 */
	/** FIXME: PROFILING? You'll know better **/
	void reassign_graph(MPI_Comm comm, int p2, Graph *pGraph, int x, MPI_Comm *newComm) {

		Graph& graph = *pGraph;
		int w, n, w2, n2, size;
		char *sendApplicationBuffer;
		int sendSizeSlice, sendBuffSize;
		bool secondSending = false;
		bool *reception;
		int nSenders;
		MPI_Request *request;
		MPI_Request request1, request2;
		MPI_Datatype blockType, oldType = MPI_LONG; //rowType is the MPI type defining what to send (the partially unpadded row).

		int p;
		MPI_Comm_size(comm, &p);

		int rank; //The rank of the current processor in the communicator.
		MPI_Comm_rank(comm, &rank);

		w = graph.get_rows_per_slice();
		n = w*p;
		w2 = (int) ceil( ((double) x)/(double (p2)) );
		n2 = w2*p2;
		size = ((int) floor( ((double) w2)/(double (w)) )) + 2; //A receiver can receive from at most size processes.
		nSenders = (int) ceil( ((double) n2)/(double (w)) );

		reception = new bool[size](); //Initialize to false.
		request = new MPI_Request[size];

		int position = rank*w;
		if(rank < nSenders) { //Senders.
			int nRows, dest;
			long *toSend = graph.begin(); //It points to what is going to be sent.
			//MPI_Type_vector(w+1, n2, n, oldType, &blockType); //Type for the whole slice.
			MPI_Pack_size(w*n2, oldType, comm, &sendSizeSlice); //Size for the whole slice part to be sent.
			sendBuffSize = sendSizeSlice + 2*MPI_BSEND_OVERHEAD; //Max size of buffer, considering 2 max sends.
			sendApplicationBuffer = new char[sendBuffSize]; //Buffer (in bytes).
			MPI_Buffer_attach(sendApplicationBuffer, sendBuffSize);
			//MPI_Type_free(&blockType);

			dest = (int) floor( ((double) position)/(double (w2)) );
			nRows = w2 - (position % w2);

			if(nRows < w && (dest + 1) < p2) {//Send to two processes.
				secondSending = true;


				// Do we do this a lot? Basically log(size of dense group)?
				// These are collectives, may be worth optimizing
				MPI::Type_vector(nRows, n2, n, oldType, &blockType); //Re-define the type for the partially unpadded row.
				MPI::Type_commit(&blockType);
				MPI::Ibsend(toSend, 1, blockType, dest, SLICETAG, comm, &request1);
				MPI::Type_free(&blockType);

				dest++; //Move to the next destination. It can send to at most 2 processes.
				toSend += nRows*n; //Advance the pointer.
				nRows = w - nRows;
				MPI::Type_vector(nRows, n2, n, oldType, &blockType); //Re-define the type for the partially unpadded row.
				MPI::Type_commit(&blockType);
				MPI::Ibsend(toSend, 1, blockType, dest, SLICETAG, comm, &request2);
				MPI::Type_free(&blockType);
			}
			else {//Send just to one.
				if(nRows > w) nRows = w; //This process has to send all of his locally stored graph.

				MPI::Type_vector(nRows, n2, n, oldType, &blockType); //Re-define the type for the partially unpadded row.
				MPI::Type_commit(&blockType);
				MPI::Ibsend(toSend, 1, blockType, dest, SLICETAG, comm, &request1);
				MPI::Type_free(&blockType);
			}
		}

		if(rank < p2) { //Receivers.
			int source, nRows;
			long *slice_memory = new long[n2*w2];
			long *toReceive = slice_memory; ////It points to where the incoming data's going to be stored.

			source = (int) floor( ((double) rank*w2)/(double (w)) );
			if(rank*w2 % w == 0) nRows = w2 - (source*w % w2);
			else nRows = w - (w2 - (source*w % w2));
			reception[0] = true;


			for(int i = 0; i < size && source < nSenders; i++) {
				if(nRows <= w && i != 0) {
					MPI::Irecv(toReceive, nRows*n2, oldType, source, SLICETAG, comm, request + i);
					break;
				}
				else {
					if(source + 1 < nSenders) reception[i + 1] = true;
					if(i == 0) {
						if(nRows > w) nRows = w;
					} else nRows = w;

					MPI::Irecv(toReceive, nRows*n2, oldType, source, SLICETAG, comm, request + i);

					source++; //Update source.
					toReceive += nRows*n2; //Advance the pointer.
					nRows = w2 - (source*w % w2);
				}
			}

			//Wait for data to be sent.
			MPI::Wait(&request1, MPI_STATUS_IGNORE);
			if(secondSending) MPI::Wait(&request2, MPI_STATUS_IGNORE);

			//Wait for data to be arrived.
			for(int z = 0; z < size; z++) {
				if(reception[z]) MPI::Wait(request + z, MPI_STATUS_IGNORE);
				else break;
			}

			Graph newSlice (x, w2, rank, n2, slice_memory);
			*pGraph = newSlice;

			MPI_Comm_split(comm, 0, 0, newComm);

		}
		else {
			//Wait for data to be sent.
			if (rank < nSenders) {
				MPI::Wait(&request1, MPI_STATUS_IGNORE);
				if(secondSending) MPI::Wait(&request2, MPI_STATUS_IGNORE);
				//graph.free_slice(); //This will be freed in recursive_contract through assigments.
			}

			MPI_Comm_split(comm, 1, 0, newComm);
		}

		MPI::Barrier(comm);

		if (rank < nSenders) {
			MPI_Buffer_detach(sendApplicationBuffer, &sendBuffSize);
			delete[] sendApplicationBuffer;
		}

		delete[] reception;
		delete[] request;

		return;
	}

}
