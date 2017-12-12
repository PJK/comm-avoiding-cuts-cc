#ifndef PARALLEL_MINIMUM_CUT_CLICK_HPP
#define PARALLEL_MINIMUM_CUT_CLICK_HPP

#include <vector>
#include <random>
#include "../AdjacencyListGraph.hpp"

namespace CLICK {
	// TODO: this should be refactored to be an 'input' class
	static void generateSlice(std::vector<AdjacencyListGraph::Edge> & edges_slice,
					   unsigned initial_edge_count,
					   unsigned group_size,
					   int rank,
					   int32_t seed_with_offset,
					   unsigned vertex_count)
	{
		/** Number of clusters */
		unsigned s = 10;
		bool last = rank == group_size;

		// Work division via the enumeration trick
		unsigned slice_portion = initial_edge_count / group_size;
		unsigned slice_from = slice_portion * rank;
		// The last node takes any leftover edges
		unsigned slice_to = last ? initial_edge_count : slice_portion * (rank + 1);

		std::mt19937 gen(static_cast<unsigned>(seed_with_offset));
		std::normal_distribution<float> mates(8, 4);
		std::normal_distribution<float> non_mates(4, 4);

		// If it works, it aint stupid, stupid. Enumerate edges.....
		unsigned long edge_ctr = 0;

		for (unsigned int i = 0; i < vertex_count; i++) {
			for (unsigned int j = i + 1; j < vertex_count; j++) {
				if (edge_ctr >= slice_from && edge_ctr < slice_to) {
					if (i % s == j % s) {
						// Same cluster
						edges_slice.push_back({ i, j, AdjacencyListGraph::Weight(std::max(0.f, mates(gen)))});
					} else {
						edges_slice.push_back({ i, j, AdjacencyListGraph::Weight(std::max(0.f, non_mates(gen))) });
					}
				}
				edge_ctr++;
			}
		}

		assert(edges_slice.size() == slice_to - slice_from);
	}
}

#endif //PARALLEL_MINIMUM_CUT_CLICK_HPP
