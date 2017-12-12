//
//  parallel_contract.hpp
//  
//
//  Created by Lukas Gianinazzi on 13.10.15.
//
//

#ifndef parallel_contract_hpp
#define parallel_contract_hpp

#include "mpi.h"
#include "prng_engine.hpp"
#include "graph_slice.hpp"

namespace mincut {
    
    void parallel_contract(MPI_Comm comm, graph_slice<long> * graph, sitmo::prng_engine * random_generator, int target_vertices);
    
}

#endif /* parallel_contract_hpp */
