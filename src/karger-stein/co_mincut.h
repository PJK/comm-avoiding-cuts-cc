//
//  CO-MinCut.h
//  
//
//  Created by Lukas Gianinazzi on 13.03.15.
//
//

#ifndef ____CO_MinCut__
#define ____CO_MinCut__

#include <stdio.h>
#include "lazy_adjacency_matrix.hpp"
#include "sparse_graph.hpp"
#include <thread>
#include "lazy_adjacency_matrix.hpp"
#include "indexed_adjacency_matrix.hpp"
#include "test/testing_utility.h"
#include "co_mincut_base_case.hpp"


namespace comincut {
    const int recursive_fanout = 2;
    const int base_case_size = 128;
    const int sparse_base_case_size = 512;

    int target_number_of_vertices_for_contraction(int capacity);

    int depth_of_recursion(int number_of_vertices);

    double min_success_in_one_trial(int number_of_vertices);

    //returns an upperbound on the number of trials needed to get the desired success probability
    int number_of_trials(int number_of_vertices, double success_probability);

    //Dense random contraction
    //Writes a lowerbound on the success probability to success
    template<class T>
    void contract_graph(indexed_adjacency_matrix<T> * graph, sitmo::prng_engine * random_engine, stack_allocator * stack, int target_number_of_vertices, T upper_bound, double * success) {

        assert (target_number_of_vertices >= 2);

        int number_of_random_contractions = graph->capacity()-target_number_of_vertices;

        int pr1_counter = 0;

        while (number_of_random_contractions > 0) {
            T contracted_weight = graph->contract_random_edge(random_engine);

            if (contracted_weight >= upper_bound) {//if the contracted edge has capacity at least the upper bound, then we can be sure that the edge was contracted successfully
                ++pr1_counter;
            }
            --number_of_random_contractions;
        }

        long long safe_capacity = graph->capacity()-pr1_counter;//since the pr1_edges are safely contracted, they can be deduced from the capacity [see probability analysis]
        //this follows by assuming the worst case in terms of success probability, which is that the safe contractions all happened first, before any of the other contrations

        *success = (double)((long long)target_number_of_vertices*((long long)target_number_of_vertices-1)) / (double)(safe_capacity * (safe_capacity-1) );

        if (pr1_counter > 0) {
            graph->pr_pass(upper_bound);
        }

        graph->compact(stack);
    }


    //Sparse random contract. Called Compact in the original paper
    //See [Karger-Stein 1996 A New Approach ..., Section 6]. This routine is called Compact in the original paper
    //Uses a random permutation of the edges to contract the graph
    //The current list of edges and their scores is stored in contraction_graph

    template <class T>
    void sparse_contract_edges(sparse_graph<T, edge<T>> * graph, sparse_graph<float, scored_edge> * contraction_graph, stack_allocator * stack, int target_number_of_vertices) {

        assert(graph->number_of_vertices() == contraction_graph->number_of_vertices());

        //if the number of vertices is exactely target_number_of_vertices or there are no edges we are done
        if (graph->number_of_vertices() == target_number_of_vertices || contraction_graph->number_of_edges() == 0) {
            return;
        }

        stack_state stack_record = stack->enter();

        //Otherwise:

        //graph->connected_components(&number_of_components, (int*)auxiliary_storage, (void*)((int*)auxiliary_storage+graph->number_of_vertices()));
        //assert(number_of_components == 1); //graph has to be connected

        std::nth_element(contraction_graph->edges.address(0), contraction_graph->edges.address(contraction_graph->number_of_edges()/2), contraction_graph->edges.end(), scored_edge::compare_score);

        //find connected components of left half
        sparse_graph<float, scored_edge> left_half = contraction_graph->prefix_graph((contraction_graph->number_of_edges()+1)/2);

        array<vertex_pair> labels = stack->allocate<vertex_pair>(graph->number_of_vertices());

        int number_of_components = left_half.connected_components(&labels, stack);

        //if the number of components is less than target_number_of_vertices, recurse left
        if (number_of_components < target_number_of_vertices) {

            stack->leave(stack_record);

            sparse_contract_edges(graph, &left_half, stack, target_number_of_vertices);

        } else {

            //otherwise, contract all edges in the right half according to their label from the connected components computation, discard the edges in the left half (the new graph has the vertex labels from the connected components computation)
            //recurse on the contracted graph

            sparse_graph<float, scored_edge> right_half = contraction_graph->suffix_graph(left_half.number_of_edges());

            graph->relabel(labels);
            right_half.relabel(labels);

            assert (graph->number_of_vertices() == number_of_components);

            stack->leave(stack_record);

            sparse_contract_edges(graph, &right_half, stack, target_number_of_vertices);
        }


    }

    //Sparse random contract
    //See [Karger-Stein 1996 A New Approach ..., Section 6]
    //Uses a random permutation of the edges to contract the graph

    template <class T>
    void contract_graph(sparse_graph<T, edge<T>> * graph, sitmo::prng_engine * random_engine, stack_allocator * stack, int target_number_of_vertices, T upper_bound, double * success) {

        stack_state stack_record = stack->enter();

        sparse_graph<float, scored_edge> contraction_graph(graph->number_of_vertices(), graph->number_of_edges(), stack);

        //generate scores
        for (int i=0;  i<graph->number_of_edges(); ++i) {
            assert(graph->edges[i].weight > 0);
            std::exponential_distribution<float> score_distribution = std::exponential_distribution<float>(graph->edges[i].weight);

            contraction_graph.edges[i].set_vertices(graph->edges[i]);
            contraction_graph.edges[i].score = score_distribution(*random_engine);
        }

        //find prefix of edges that leads to target_number_of_vertices components and contract those edges
        sparse_contract_edges(graph, &contraction_graph, stack, target_number_of_vertices);

        stack->leave(stack_record);

        graph->pr_pass(upper_bound, stack);

        *success = 0.5;//no heuristic
    }


    template <class T>
    T recursive_contraction(indexed_adjacency_matrix<T> * graph, sitmo::prng_engine * random_engine, stack_allocator * stack, T upper_bound, bool preserve_input_graph, double * success_probability);

    template <class T>
    T recursive_contraction(sparse_graph<T, edge<T>> * graph, sitmo::prng_engine * random_engine, stack_allocator * stack, T upper_bound, bool preserve_input_graph, double * success_probability);

    template <class T>
    double minimum_cut_trials_start(adjacency_matrix<T> * graph, T * degrees, T * result, int seed, double min_success);


    ///
    //Recursively computes an upper bound on the minimum cut value
    //Stores a lower bound on the correctness probability in success_probability
    //See [Karger-Stein 1996 A New Approach ...] for an overview
    //Uses random_engine to generate the random numbers, stack to allocate memory
    //A precondition is that upper_bound is an upperbound on the minimum cut value of the graph
    //The GraphT type must be such that contract_graph can be used with it (either sparse_graph<T, edge<T>> or indexed_adjacency_matrix<T>)

    template <class T, class GraphT>
    T recursive_contraction_inner(GraphT * graph, sitmo::prng_engine * random_engine, stack_allocator * stack, T upper_bound, bool preserve_input_graph, double * success_probability) {

        stack_state saved_stack_state = stack->enter();

        assert (graph->number_of_vertices() > base_case_size);

        double success_probabilities[recursive_fanout];
        double contraction_success[recursive_fanout];

        const int target_vertices = target_number_of_vertices_for_contraction(graph->number_of_vertices());

        for (int i=0; i<recursive_fanout; ++i) {

            T cut;

            if (i==recursive_fanout-1 && !preserve_input_graph) {
                //in the last iteration we can destroy the input graph
                contract_graph(graph, random_engine, stack, target_vertices, upper_bound, &contraction_success[i]);

                cut = recursive_contraction<T>(graph, random_engine, stack, upper_bound, false, &success_probabilities[i]);

            } else {

                stack_state saved_stack_state_inner = stack->enter();

                //in all the other iterations, create a copy of the input graph before contracting
                GraphT contracted_graph(graph, stack);

                contract_graph(&contracted_graph, random_engine, stack, target_vertices, upper_bound, &contraction_success[i]);

                assert (contracted_graph.number_of_vertices() <= target_vertices);

                cut = recursive_contraction<T>(&contracted_graph, random_engine, stack, upper_bound, false, &success_probabilities[i]);


                stack->leave(saved_stack_state_inner);
            }

            upper_bound = std::min(upper_bound, cut);
        }

        *success_probability = (double)1.0 - ((double)1.0 - contraction_success[0] * success_probabilities[0]) * ((double)1.0 - contraction_success[1] * success_probabilities[1]);

        stack->leave(saved_stack_state);

        return upper_bound;
    }

    ///
    //Recursively computes an upper bound on the minimum cut value
    //Stores a lower bound on the correctness probability in success_probability
    //See [Karger-Stein 1996 A New Approach ...] for an overview
    //Uses random_engine to generate the random numbers, stack to allocate memory
    //A precondition is that upper_bound is an upperbound on the minimum cut value of the graph
    //The GraphT type must be such that contract_graph can be used with it (either sparse_graph<T, edge<T>> or indexed_adjacency_matrix<T>)

    template <class T, class GraphT>
    T recursive_contraction_half(GraphT * graph, sitmo::prng_engine * random_engine, stack_allocator * stack, T upper_bound) {

        stack_state saved_stack_state = stack->enter();

        double contraction_success;

        const int target_vertices = target_number_of_vertices_for_contraction(graph->number_of_vertices());


        if (graph->number_of_vertices() <= base_case_size) {//Base case
            upper_bound = deterministic_cut_pr(graph, stack, upper_bound);

        } else {

            contract_graph(graph, random_engine, stack, target_vertices, upper_bound, &contraction_success);

            for (int i=0; i<recursive_fanout; ++i) {
                stack_state saved_stack_state2 = stack->enter();

                GraphT contracted_graph(graph, stack);

                T cut = recursive_contraction_half<T>(&contracted_graph, random_engine, stack, upper_bound);

                upper_bound = std::min(upper_bound, cut);

                stack->leave(saved_stack_state2);
            }
        }

        stack->leave(saved_stack_state);

        return upper_bound;
    }



    template <class T>
    T recursive_contraction(indexed_adjacency_matrix<T> * graph, sitmo::prng_engine * random_engine, stack_allocator * stack, T upper_bound, bool preserve_input_graph, double * success_probability) {
        if (graph->number_of_vertices() <= base_case_size) {
            *success_probability = 1.0;
            return deterministic_cut_pr<T>(graph, stack, upper_bound);
        }

        return recursive_contraction_inner<T, indexed_adjacency_matrix<T>>(graph, random_engine, stack, upper_bound, preserve_input_graph, success_probability);
    }

    template <class T>
    T recursive_contraction(sparse_graph<T, edge<T>> * graph, sitmo::prng_engine * random_engine, stack_allocator * stack, T upper_bound, bool preserve_input_graph, double * success_probability) {

        if (graph->number_of_edges() <= sparse_base_case_size || graph->number_of_vertices() <= base_case_size) {
            *success_probability = 1.0;
            return deterministic_cut_pr<T>(graph, stack, upper_bound);
        }

        /*
        if (graph->number_of_edges() >= graph->number_of_vertices()*((long)graph->number_of_vertices()/16)) {

            stack_state saved_stack_state = stack->enter();

            array<T> degrees = stack->allocate<T>(graph->number_of_vertices());
            adjacency_matrix<T> dense_graph = graph->as_dense_graph(stack);
            dense_graph.compute_degrees(degrees);
            indexed_adjacency_matrix<T> indexed_graph(&dense_graph, degrees, stack);

            T result = recursive_contraction_inner<T, indexed_adjacency_matrix<T>>(&indexed_graph, random_engine, stack, upper_bound, false, success_probability);

            stack->leave(saved_stack_state);

            return result;

        } else {*/
        return recursive_contraction_inner<T, sparse_graph<T, edge<T>>>(graph, random_engine, stack, upper_bound, preserve_input_graph, success_probability);
        //}

    }


    ////
    //Performs as many trials as necessary to guarantee with probability min_success that the result is the mincut value
    ////

    template <class T, class graphT>
    T minimum_cut_trials(graphT * graph, stack_allocator * stack, T upper_bound, int seed, double min_success) {

        //Parallel random number generator assures that if seeds for this call are different, then the streams produced are independent
        sitmo::prng_engine random_engine(seed);

        T min_so_far = upper_bound;

        const int max_trials = number_of_trials(graph->number_of_vertices(), min_success);
        assert (max_trials > 0);
        //double failure_so_far = 1.0;
        
        //int vertices = graph->number_of_vertices();

        for (int i=0; i<max_trials; ++i) {
            double ad_hoc_success;

            T cut = recursive_contraction<T> (graph, &random_engine, stack, min_so_far, true, &ad_hoc_success);

            min_so_far = std::min(min_so_far, cut);

            //failure_so_far = failure_so_far * (1.0-ad_hoc_success);

            //std::cout << "c number of vertices " << vertices << ", mincut so far " << cut << ", ad hoc success " << ad_hoc_success << " failure so far " << failure_so_far << std::endl;
        }

        return min_so_far;
    }

    template <class T>
    void minimum_cut_trials_start_try(adjacency_matrix<T> * graph, T * result, int seed) {

        stack_allocator * stack = new stack_allocator(0);

        array<T> capacities = stack->allocate<T>(graph->number_of_vertices());

        graph->compute_degrees(capacities);

        indexed_adjacency_matrix<T> indexed_graph(graph, capacities, stack);

        T min_so_far = *std::min_element(capacities.begin(), capacities.end());

        //std::cout << "cheapest 1 vertex cut " << min_so_far << std::endl;

        sitmo::prng_engine random_engine(seed);

        *result = recursive_contraction_half<T, indexed_adjacency_matrix<T>>(&indexed_graph, &random_engine, stack, min_so_far);

        delete stack;
    }

    ///
    //allocates the call stack and converts the graph into the correct representation (for dense input graph)
    ///

    template <class T>
    void minimum_cut_trials_start(adjacency_matrix<T> * graph, T * result, int seed, double min_success) {

        stack_allocator * stack = new stack_allocator(0);

        array<T> capacities = stack->allocate<T>(graph->number_of_vertices());

        graph->compute_degrees(capacities);

        indexed_adjacency_matrix<T> indexed_graph(graph, capacities, stack);

        T min_so_far = *std::min_element(capacities.begin(), capacities.end());

        //std::cout << "cheapest 1 vertex cut " << min_so_far << std::endl;

        min_so_far = minimum_cut_trials(&indexed_graph, stack, min_so_far, seed, min_success);

        *result = min_so_far;

        delete stack;
    }

    ///
    //allocates the call stack and converts the graph into the correct representation (for sparse input graph)
    ///

    template <class T>
    void minimum_cut_trials_start(sparse_graph<T, edge<T>> * graph, T * result, int seed, double min_success) {

        stack_allocator * stack = new stack_allocator(0);

        if (graph->number_of_edges() >= (long long)graph->number_of_vertices() * graph->number_of_vertices() / 32) {

            adjacency_matrix<T> dense_graph = graph->as_dense_graph(stack);

            minimum_cut_trials_start(&dense_graph, result, seed, min_success);
        } else {

            //stack->reserve(128 * (graph->number_of_edges()+graph->number_of_vertices()) * (long)(depth_of_recursion(graph->number_of_vertices()+1)));

            *result = minimum_cut_trials<T>(graph, stack, std::numeric_limits<T>::max(), seed, min_success);
        }

        delete stack;
    }

    ////
    //Calls num_threads batches of trials in parallel (if num_threads = 1, no additional thread is created)
    ////

    template <class T, class graphT>
    T minimum_cut_trials_parallelize(graphT * graph, int seed, double min_success, int num_threads) {

        double success_per_thread = 1-(pow(1-min_success, 1.0/num_threads));

//#ifndef NDEBUG
//        std::cout << "success per thread " << success_per_thread << std::endl;
//#endif

        std::vector<std::thread> threads;

        T * results = new T[num_threads];

        for (int i=0; i<num_threads-1; ++i) {
            threads.push_back(std::thread([=] {
                minimum_cut_trials_start<T>(graph, results+i, seed+i, success_per_thread);
            }));
        }

        minimum_cut_trials_start<T>(graph, &results[num_threads-1], seed+num_threads-1, success_per_thread);

        for(auto& thread : threads){
            thread.join();
        }

        T min = *std::min_element(results, results+num_threads);

        delete[] results;
        //delete threads;
        //free(threads);

        return min;
    }


    ////
    //Public Functions
    ////

    
    /**
     * Computes an upperbound on the minimum cut of a densely represented graph.
     * Use comincut::number_of_trials to determine how many trials are needed
     * in order to guarantee the desired probability of finding the minimum cut.
     * The seed is used to initialize the internal prng.
     */
    template <class T>
    T minimum_cut_try(adjacency_matrix<T> * graph, int seed) {

        T cut = 0;

        minimum_cut_trials_start_try(graph, &cut, seed);

        return cut;
    }

    
    /**
     * Computes the minimum cut of a densely represented graph.
     * The result is correct with at least min_success probability.
     * The seed is used to initialize the internal prng.
     */
    template <class T>
    T minimum_cut(adjacency_matrix<T> * graph, double min_success, int seed) {

        T cut = 0;

        minimum_cut_trials_start(graph, &cut, seed, min_success);

        return cut;
    }
    
    template <class T>
    bool is_connected(sparse_graph<T, edge<T>> * graph) {
        
        stack_allocator stack(0);
        
        array<vertex_pair> labels = stack.allocate<vertex_pair>(graph->number_of_vertices());
        
        int number_of_components = graph->connected_components(&labels, &stack);
        
        if (number_of_components != 1) {
            std::cout << "graph has " << number_of_components << " components: " << std::endl;
            std::cout << *graph << std::endl;
        }
        
        return number_of_components == 1;
    }
    
    /**
     * Computes the minimum cut of a sparsely represented graph.
     * The result is correct with at least min_success probability.
     * The seed is used to initialize the internal prng.
     */
    template <class T>
    T minimum_cut(sparse_graph<T, edge<T>> * graph, double min_success, int seed) {
        
        assert (is_connected(graph));
        
        T mincut;
        minimum_cut_trials_start(graph, &mincut, seed, min_success);
        
        return mincut;
    }

    /**
     * Computes the minimum cut of a sparsely represented graph.
     * The result is deterministic.
     */
    template <class T>
    T deterministic_minimum_cut(sparse_graph<T, edge<T>> * graph) {
        
        assert (is_connected(graph));
        
        stack_allocator * stack = new stack_allocator(0);

        T result = deterministic_cut_pr<T>(graph, stack, std::numeric_limits<T>::max());

        delete stack;

        return result;
    }
}

#endif /* defined(____CO_MinCut__) */
