//
// Created by Pavel Kalvoda on 28/10/15.
//

#include "connected_components.hpp"
#include "disjoint_sets.hpp"

using boost::disjoint_sets;
using std::vector;

/** FIXME: PROFILING **/
// This is a sequential bottleneck, if we wanted to tune, this would be one place
// But I believe it should be reasonably fast -- it's simple
bool prefix_connected_components(const edge_struct_t * edges, size_t edge_count, int * vertices_map, size_t vertex_count, size_t components_count, size_t * prefix_length)
{
	DisjointSets<int> dsets(vertex_count);

	if (components_count == 0 || vertex_count == 0) {
		*prefix_length = 0;
		return true;
	}

	size_t components_active = vertex_count;
	bool found = false;

    size_t i = 0;
	for (; i < edge_count && components_active > components_count; i++) {
		int v1_set = dsets.find(edges[i].v1),
			v2_set = dsets.find(edges[i].v2);
		if (v1_set != v2_set) {
			components_active--;
			dsets.unify(v1_set, v2_set);
		}
	}
    
    if (components_active == components_count) {
        *prefix_length = i + 1;
        found = true;
    }

	for (size_t j = 0; j < vertex_count; j++)
		vertices_map[j] = dsets.find((int) j);

	if(!found)
		*prefix_length = edge_count;

	return found;
}
