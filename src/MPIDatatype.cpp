#include "MPIDatatype.hpp"

bool MPIDatatype<AdjacencyListGraph::Edge>::initialized = false;
MPI_Datatype MPIDatatype<AdjacencyListGraph::Edge>::edge_type;

bool MPIDatatype<UnweightedGraph::Edge>::initialized = false;
MPI_Datatype MPIDatatype<UnweightedGraph::Edge>::edge_type;
