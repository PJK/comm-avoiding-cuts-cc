#include "ApproximateCut.hpp"
#include "UnweightedGraph.hpp"
#include "utils.hpp"
#include "MPICollector.hpp"
#include "IteratedSparseSampling.hpp"
#include "UnweightedIteratedSparseSampling.hpp"
#include <algorithm>
#include <random>

ApproximateCut::Result ApproximateCut::run(double success_probability) {
	Result result;

	MPI_Barrier(communicator_);

	TimeUtils::measure<void>([&]() {
		MPI::total = 0; // Just to be sure

		// Phase 1 : generate upperbound on the mincut
		AdjacencyListGraph::Weight w = std::accumulate(
				edges_.begin(),
				edges_.end(),
				AdjacencyListGraph::Weight(0),
				[](AdjacencyListGraph::Weight state, AdjacencyListGraph::Edge & edge) { return state + edge.weight; }
		);

		AdjacencyListGraph::Weight weightUpperBound;
		MPI::Reduce(&w, &weightUpperBound, 1, MPIDatatype<AdjacencyListGraph::Weight>::constructType(), MPI_SUM, 0, communicator_);

		// Phase 2 : try different subgraphs and test connectivity.
		unsigned iterations = (unsigned) std::ceil(std::log(w));
		unsigned trials = numberOfTrials(vertex_count_, success_probability); //O(log n)
		sitmo::prng_engine random_engine(seed_);
		int first_disconnected = 0;

		for (unsigned i = 1; i <= iterations; i++) {
			double sample_inv = std::pow(2, -int(i));
			std::vector<UnweightedGraph::Edge> subgraph;

			for (int j = 0; j < trials; ++j) {
				for (AdjacencyListGraph::Edge edge : edges_) {
					std::binomial_distribution<int> distribution(edge.weight, sample_inv);

					if (distribution(random_engine) > 0) {
						// Relabel edges to make graphs from different trials disjunct
						subgraph.push_back({edge.from + j * vertex_count_, edge.to + j * vertex_count_});
					}
				}
			}

			//Test Connectivity
			UnweightedIteratedSparseSampling component_finder(
					communicator_,
					0,
					p_,
					random_engine(),
					1,
					vertex_count_ * trials,
					subgraph.size()
			);

			component_finder.setSlice(subgraph);

			std::vector<unsigned> connected_components;
			int num_components = component_finder.connectedComponents(connected_components);

			if (num_components > trials) {
				first_disconnected = i; // we've reached the O(log n) area around the minimum cut value.
				break;
			}
		}

		result.weight = (unsigned long) std::pow(2, first_disconnected);
		result.trials = trials * first_disconnected;
	}, result.cuttingTime);

	MPI_Reduce(&MPI::total, &result.mpiTime, 1, MPI_DOUBLE, MPI_MAX, 0, communicator_);

	return result;
}


unsigned ApproximateCut::numberOfTrials(unsigned n, double success_probability) const {
	double alpha = std::log(1-std::exp(-double(0.75)));
	double trials = std::log(double(1.0)-success_probability)/alpha;

	return (unsigned)std::ceil(trials);
}
