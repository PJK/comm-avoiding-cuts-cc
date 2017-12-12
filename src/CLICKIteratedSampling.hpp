#ifndef PARALLEL_MINIMUM_CUT_CLICKITERATEDSAMPLING_HPP
#define PARALLEL_MINIMUM_CUT_CLICKITERATEDSAMPLING_HPP

#include "WeightedIteratedSparseSampling.hpp"
#include "AdjacencyListGraph.hpp"
#include "input/CLICK.hpp"


/**
 * Overrides the input loading with in-memory CLICK model generation of a complete graph.
 * The parameters are built in since we don't have a suitable interface to pass them around ATM
 */
class CLICKIteratedSampling : public WeightedIteratedSparseSampling {

public:

	CLICKIteratedSampling(MPI_Comm communicator,
						  int color,
						  int group_size,
						  int32_t seed_with_offset,
						  unsigned vertex_count,
						  unsigned target_size) :
			WeightedIteratedSparseSampling(
					communicator,
					color,
					group_size,
					seed_with_offset,
					target_size,
					vertex_count,
					vertex_count * (vertex_count - 1) / 2
			)
	{}

	virtual void loadSlice() {
		CLICK::generateSlice(
				edges_slice_,
				initial_edge_count_,
				group_size_,
				rank_,
				seed_with_offset_,
				vertex_count_
		);
	}

	const std::vector<AdjacencyListGraph::Edge> & edgeSlice() const {
		return edges_slice_;
	}
};


#endif //PARALLEL_MINIMUM_CUT_CLICKITERATEDSAMPLING_HPP
