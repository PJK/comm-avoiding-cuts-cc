//
// Created by Pavel Kalvoda on 28/10/15.
//

#ifndef PARALLEL_MINIMUM_CUT_CONNECTED_COMPONENTS_HPP
#define PARALLEL_MINIMUM_CUT_CONNECTED_COMPONENTS_HPP


#include "graph_slice.hpp"

/**
 * @param edges array of {edge_count >= 0} edges
 * @param[out] vertices_map preallocated map of {vertex_count >= 0} vertices. Will be filled with partitions label from [0, vertex_count)
 * @param components_count the desired number of connected components
 * @param[out] prefix_length length (>= 0) of the prefix of {edges} that induces a graph with {components_count} components, or {edge_count}
 * 		if no such prefix exists
 * @return true if the described prefix exists
 */
bool prefix_connected_components(const edge_struct_t * edges, size_t edge_count, int * vertices_map, size_t vertex_count, size_t components_count, size_t * prefix_length);

#endif //PARALLEL_MINIMUM_CUT_CONNECTED_COMPONENTS_HPP
