#ifndef PARALLEL_MINIMUM_CUT_MPIDATATYPE_HPP
#define PARALLEL_MINIMUM_CUT_MPIDATATYPE_HPP

#include <mpi.h>
#include "AdjacencyListGraph.hpp"
#include "UnweightedGraph.hpp"

/**
 * Trait container to allow external implementation for any type
 */
template <typename ElementType>
struct MPIDatatype {};

template <>
struct MPIDatatype<int> {
	static MPI_Datatype constructType() {
		return MPI_INT;
	}
};

template<>
struct MPIDatatype<AdjacencyListGraph::Weight> {
	static MPI_Datatype constructType() {
		return MPI_UNSIGNED_LONG;
	}
};

template <>
struct MPIDatatype<AdjacencyListGraph::Edge> {
	// TODO global registry -- free datatype
	static MPI_Datatype edge_type;
	static bool initialized;

	static MPI_Datatype constructType() {
		if (!initialized) {
			int blocklengths[3] = { 1, 1, 1 };

			// This leaks abstraction :(
			MPI_Datatype types[3] = { MPI_UNSIGNED, MPI_UNSIGNED, MPIDatatype<AdjacencyListGraph::Weight>::constructType() };

			MPI_Aint offsets[3];

			offsets[0] = offsetof(AdjacencyListGraph::Edge, from);
			offsets[1] = offsetof(AdjacencyListGraph::Edge, to);
			offsets[2] = offsetof(AdjacencyListGraph::Edge, weight);

			MPI_Type_create_struct(3, blocklengths, offsets, types, &edge_type);
			MPI_Type_commit(&edge_type);

			initialized = true;
		}
		return edge_type;
	};
};

template <>
struct MPIDatatype<UnweightedGraph::Edge> {
	// TODO global registry -- free datatype
	static MPI_Datatype edge_type;
	static bool initialized;

	static MPI_Datatype constructType() {
		if (!initialized) {
			int blocklengths[2] = { 1, 1 };

			// This leaks abstraction :(
			MPI_Datatype types[2] = { MPI_UNSIGNED, MPI_UNSIGNED };

			MPI_Aint offsets[2];

			offsets[0] = offsetof(UnweightedGraph::Edge, from);
			offsets[1] = offsetof(UnweightedGraph::Edge, to);

			MPI_Type_create_struct(2, blocklengths, offsets, types, &edge_type);
			MPI_Type_commit(&edge_type);

			initialized = true;
		}
		return edge_type;
	};
};

#endif //PARALLEL_MINIMUM_CUT_MPIDATATYPE_HPP
