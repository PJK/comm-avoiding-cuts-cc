#ifndef PARALLEL_MINIMUM_CUT_SEQUENTIALSQUAREROOTCUT_HPP
#define PARALLEL_MINIMUM_CUT_SEQUENTIALSQUAREROOTCUT_HPP

#include "AdjacencyListGraph.hpp"
#include "prng_engine.hpp"
#include "sum_tree.hpp"
#include "sparse_graph.hpp"
#include "stack_allocator.h"
#include "co_mincut.h"


/**
 * Performs a single sqrt(m)-cut trial
 */
class SequentialSquareRootCut {
	AdjacencyListGraph * graph_;
	sitmo::prng_engine * random_;
	unsigned int target_size_;

public:
	SequentialSquareRootCut(AdjacencyListGraph * graph, sitmo::prng_engine * random, unsigned int target_size) :
			graph_(graph), random_(random), target_size_(target_size) {
		
	}

	AdjacencyListGraph::Weight compute();

protected:
    
	void iteratedSampling(unsigned final_size);
};


#endif //PARALLEL_MINIMUM_CUT_SEQUENTIALSQUAREROOTCUT_HPP
