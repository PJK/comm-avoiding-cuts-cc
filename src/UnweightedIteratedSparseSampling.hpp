#ifndef PARALLEL_MINIMUM_CUT_UNWEIGHTEDITERATEDSPARSESAMPLING_HPP
#define PARALLEL_MINIMUM_CUT_UNWEIGHTEDITERATEDSPARSESAMPLING_HPP

#include "IteratedSparseSampling.hpp"
#include "GraphInputIterator.hpp"
#include "UnweightedGraph.hpp"

class UnweightedIteratedSparseSampling : public IteratedSparseSampling<UnweightedGraph::Edge> {
	const float epsilon_ = 0.09f;
	const float delta_ = 0.2f;

public:
	using IteratedSparseSampling<UnweightedGraph::Edge>::IteratedSparseSampling;

	/**
	 * @param [out] connected_components The root will receive the labels of the connected components in the vector
	 * @return the number of connected components of the graph.
	 */
	unsigned connectedComponents(std::vector<unsigned> & connected_components);

	virtual void loadSlice() {
		throw "TODO this is ugly now and breaks subtyping";
	}

	void loadSlice(GraphInputIterator & input);

protected:
	unsigned countEdges();

	/**
	 * @return A vector whose entries correspond to the number of edges to sample at that processor
	 *
	 * We use `int` because this vector will be used to compute MPI displacements
	 */
	std::vector<int> edgesToSamplePerProcessor(std::vector<int> edges_available_per_processor);

	virtual std::vector<UnweightedGraph::Edge> sample(unsigned edge_count);

	/**
	 * Has to be called collectively
	 * @return Edges per rank, at root only
	 */
	std::vector<int> edgesAvailablePerProcessor();
};


#endif //PARALLEL_MINIMUM_CUT_UNWEIGHTEDITERATEDSPARSESAMPLING_HPP
