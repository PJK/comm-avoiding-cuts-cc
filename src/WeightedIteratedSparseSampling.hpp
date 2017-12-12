#ifndef PARALLEL_MINIMUM_CUT_WEIGHTEDITERATEDSPARSESAMPLING_HPP
#define PARALLEL_MINIMUM_CUT_WEIGHTEDITERATEDSPARSESAMPLING_HPP

#include "IteratedSparseSampling.hpp"
#include "AdjacencyListGraph.hpp"

/**
 * Provides ISS on weighted graphs, and adds the following functionality
 *  - shrinking to target size
 *  - RC matrix construction
 */
class WeightedIteratedSparseSampling : public IteratedSparseSampling<AdjacencyListGraph::Edge> {
public:
	using IteratedSparseSampling<AdjacencyListGraph::Edge>::IteratedSparseSampling;

	/**
	 * Shrinks the graph
	 */
	void shrink();

	/**
	 * Run a single sampling trial
	 *
	 * @return Has the desired number of vertices been reached?
	 */
	bool samplingTrial();

	/**
	 * Reduce results across all nodes
	 *
	 * @return Input for recursive contract
	 */
	graph_slice<long> reduce();

	/**
	 * Sample `edge_count` edges locally, prop. to their weight
	 * @param edge_count
	 * @return The edge sample
	 */
	virtual std::vector<AdjacencyListGraph::Edge> sample(unsigned edge_count);

	/**
	 * @param weights Fills the weights array in the master (the others are left unchanged)
	 */
	void gatherWeights(std::vector<AdjacencyListGraph::Weight> & weights);

	/**
	 * XXX: This doesn't do what the name says. Ask Lukas.
	 *
	 * @param weights Fills the weights array in the master (the others are left unchanged)
	 * @return at every processor the total number of edges.
	 */
	AdjacencyListGraph::Weight countEdges(std::vector<AdjacencyListGraph::Weight> & weights);

	/**
	 * @param [out] connected_components The root will receive the labels of the connected components in the vector
	 * @return the number of connected components of the graph.
	 */
	unsigned connectedComponents(std::vector<unsigned> & connected_components);

	/**
	 * @return A vector whose entries correspond to the number of edges to sample at that processor
	 *
	 * We use `int` because this vector will be used to compute MPI displacements
	 */
	std::vector<int> edgesToSamplePerProcessor(std::vector<AdjacencyListGraph::Weight> const & weights) {
		unsigned number_of_edges_to_sample = (unsigned) std::pow((float) initial_vertex_count_, 1 + epsilon_ / 2);
		// [i] = how many i should sample
		// We use ints to allow using this for the displacements
		std::vector<int> edges_per_processor(group_size_, 0);

		sum_tree<AdjacencyListGraph::Weight> index(weights.data(), group_size_);
		AdjacencyListGraph::Weight sum = index.root();

		std::uniform_int_distribution<AdjacencyListGraph::Weight> uniform_int(1, sum);

		for (size_t i = 0; i < number_of_edges_to_sample; i++) {
			size_t selection = index.lower_bound(uniform_int(random_engine_));

			edges_per_processor[selection]++;
		}

		return edges_per_processor;
	}
};


#endif //PARALLEL_MINIMUM_CUT_WEIGHTEDITERATEDSPARSESAMPLING_HPP
