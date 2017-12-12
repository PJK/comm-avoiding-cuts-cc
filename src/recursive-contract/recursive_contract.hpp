/*
 * recursive_contract.hpp
 *
 *  Created on: 17 nov 2015
 *      Author: Studente
 */

#ifndef RECURSIVE_CONTRACT_HPP_
#define RECURSIVE_CONTRACT_HPP_

#include <cmath>
#include <limits>
#include "mpi.h"


typedef graph_slice<long> Graph; //Use graphs with long weights.

namespace mincut {

	Graph duplicate_graph(MPI_Comm comm, int p, Graph *pGraph, MPI_Comm *newComm);
	void reassign_graph(MPI_Comm comm, int p2, Graph *pGraph, int x, MPI_Comm *newComm);
    
    
    long parallel_recursive_contract(MPI_Comm comm, Graph& graph);//deprecated, only use for basic testing
    
    long parallel_recursive_contract(MPI_Comm comm, Graph& graph, sitmo::prng_engine * random);
    
    long parallel_cut(MPI_Comm comm, Graph& graph, int trials, int seed);
    
    long parallel_cut(MPI_Comm comm, Graph& graph, double min_success, int seed);

    long distributed_parallel_cut(MPI_Comm comm, int totTrials, int seed, int V, std::string filename);

    void distribute_trials(MPI_Comm comm, int *localTrials, int *subsetSize, int *leftouts, int V, int nTrials);

    Graph distribute_graph(MPI_Comm comm, Graph& graph, int subsetSize, int leftouts, MPI_Comm *subsetComm);

    long distributed_parallel_cut(MPI_Comm comm, double success, int seed, int V, std::string filename);
}

#endif /* RECURSIVE_CONTRACT_HPP_ */
