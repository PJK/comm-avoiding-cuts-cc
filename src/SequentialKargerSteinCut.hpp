#ifndef SEQUENTIAL_KARGER_STEIN_CUT_HPP
#define SEQUENTIAL_KARGER_STEIN_CUT_HPP

#include "AdjacencyListGraph.hpp"
#include "prng_engine.hpp"
#include "sum_tree.hpp"
#include "sparse_graph.hpp"
#include "stack_allocator.h"
#include "co_mincut.h"


/**
 * Finds a minimum cut using the Monte-Carlo Karger-Stein algorithm.
 * The result is correct with a given probability 'success_probablity'.
 * For a constant 'success_probablity', the runtime is O(n^2 log ^2 (n)) on a graph with n vertices.
 */
class SequentialKargerSteinCut {
    AdjacencyListGraph * graph_;
    uint32_t seed_;
    double success_probability_;
    
public:
    SequentialKargerSteinCut(AdjacencyListGraph * graph, uint32_t seed, double success_probability) :
    graph_(graph), seed_(seed), success_probability_(success_probability) {
    }
    
    //Performs a single trial. The success_probability is as given
    SequentialKargerSteinCut(AdjacencyListGraph * graph, uint32_t seed) :
    graph_(graph), seed_(seed), success_probability_(0.0) {
    }
    
    AdjacencyListGraph::Weight compute();
    
protected:

};


#endif //SEQUENTIAL_KARGER_STEIN_CUT_HPP
