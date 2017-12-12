#ifndef PARALLEL_MINIMUM_CUT_FILEITERATEDSAMPLING_HPP
#define PARALLEL_MINIMUM_CUT_FILEITERATEDSAMPLING_HPP

#include "WeightedIteratedSparseSampling.hpp"
#include "GraphInputIterator.hpp"

/**
 * Iterated sampling from file input
 */
class FileIteratedSampling : public WeightedIteratedSparseSampling {
	GraphInputIterator & input_;

public:

	FileIteratedSampling(MPI_Comm communicator,
						 int color,
						 int group_size,
						 GraphInputIterator & input,
						 int32_t seed_with_offset,
						 unsigned target_size) :
			WeightedIteratedSparseSampling(
					communicator,
					color,
					group_size,
					seed_with_offset,
					target_size,
					input.vertexCount(),
					input.edgeCount()
			),
			input_(input) {}

	virtual void loadSlice() {
		input_.loadSlice(edges_slice_, rank(), group_size_);
	}
};


#endif //PARALLEL_MINIMUM_CUT_FILEITERATEDSAMPLING_HPP
