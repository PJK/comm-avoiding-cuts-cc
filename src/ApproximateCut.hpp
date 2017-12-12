#ifndef APPROXIMATE_CUT_HPP
#define APPROXIMATE_CUT_HPP

#include <mpi.h>
#include "AdjacencyListGraph.hpp"
#include "GraphInputIterator.hpp"
#include "input/CLICK.hpp"

/**
 * Implements a O(log n) minimum cut approximation algorithm. This is the top level class
 * that abstracts away the algorithm implementation as well as MPI details.
 */
class ApproximateCut {
	MPI_Comm communicator_;
	int p_, rank_;
	std::vector<AdjacencyListGraph::Edge> edges_;
	unsigned vertex_count_, initial_edge_count_;
	uint32_t seed_;

public:

	struct Result {
		AdjacencyListGraph::Weight weight;
		unsigned trials;
		double cuttingTime, mpiTime;
	};

	/**
	 * The communicator ownership is exclusive to SquareRootCut until all members have performed
	 * a runMaster/runWorker.
	 */
	ApproximateCut(MPI_Comm comm, uint32_t seed) : communicator_(comm) {
		MPI_Comm_size(communicator_, &p_);
		MPI_Comm_rank(communicator_, &rank_);
		seed_ = seed + rank_;
	}

	bool master() const {
		return rank_ == 0;
	}

	unsigned processors() const {
		return (unsigned) p_;
	}

	unsigned initialVertexCount() const {
		return vertex_count_;
	}

	unsigned initialEdgeCount() const {
		return initial_edge_count_;
	}

	unsigned rank() const {
		return (unsigned) rank_;
	}

	/**
	 * Perform an approximate minimum cut computation
	 * Only the master (rank 0) holds a valid result.
	 */
	Result run(double success_probability);

	void loadFromInput(GraphInputIterator & input) {
		vertex_count_ = input.vertexCount();
		initial_edge_count_ = input.edgeCount();
		input.loadSlice(edges_, rank(), processors());
	}

	void loadFromCLICK(unsigned vertex_count) {
		vertex_count_ = vertex_count;
		initial_edge_count_ = vertex_count_ * (vertex_count_ - 1) / 2;

		CLICK::generateSlice(
				edges_,
				initial_edge_count_,
				(unsigned) p_,
				rank_,
				seed_,
				vertex_count_
		);
	}

protected:

	/**
	 * Gives the number of trials to achieve success_probability - O( (log^2 n) / n) success probability.
	 * That is, the probability to succeed in finding an O(log n) approximation approaches success_probability quickly as n goes to infinity.
	 * @param n
	 * @param success_probability
	 * @return
	 */
	unsigned numberOfTrials(unsigned n, double success_probability) const;
};


#endif //APPROXIMATE_CUT_HPP
