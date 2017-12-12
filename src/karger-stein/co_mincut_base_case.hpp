//
//  CO-MinCut.cpp
//  
//
//  Created by Lukas Gianinazzi on 13.03.15.
//
//

#ifndef ____CO_MinCut_implementation_base____
#define ____CO_MinCut_implementation_base____


#include "lazy_adjacency_matrix.hpp"
#include "indexed_adjacency_matrix.hpp"
#include "test/testing_utility.h"
#include "math.h"
#include <algorithm>
#include <functional> //std:: minus
#include <immintrin.h>
#include <x86intrin.h>
#include "prng_engine.hpp"
#include <thread>
#include <limits>
#include "adjacency_array.hpp"
#include "update_heap.hpp"

namespace comincut {
    template<class T, class EdgeT>
    void visit_vertex(array<bool> valid, update_heap<T> * heap, EdgeT * row_begin, EdgeT * row_end, int next_edge_index) {
        
        assert(std::is_sorted(row_begin, row_end));
        
        EdgeT * cur_row = row_begin;
        
        //remove next_edge_index
        valid.set(next_edge_index, false);
        heap->set_value(next_edge_index, 0);
        
        //add the priorities
        while (cur_row < row_end) {

            if (valid.get(cur_row->vertex2())) {
                heap->increment_value(cur_row->vertex2(), cur_row->value());
            }
            ++cur_row;
        }
        
        heap->update();
    }
    
    
    template<class T, class EdgeT>
    T deterministic_cut_sparse(adjacency_array<T, EdgeT> * graph, stack_allocator * stack, int target_number_of_vertices) {
        //Algorithm: wagner-stoer
        //time O(min(V^3, E V log V))
        //memory transfers: O(min(V^3/B, E V log (V/B)))) where B is the cache line width

        stack_state stack_record = stack->enter();
        
        int V = graph->number_of_vertices();
        
        array<bool> in_graph = stack->allocate<bool>(V);
        std::fill(in_graph.address(0), in_graph.end(), true);

        array<bool> valid = stack->allocate<bool>(V);
        
        T best_cut = std::numeric_limits<T>::max();
        
        assert (V > 1);
        
        //contract edges
        for (int i=0; i<(V-target_number_of_vertices); ++i) {
            
            stack_state stack_record_inner = stack->enter();
            
            int cur_V = (V-i);

            assert(graph->number_of_vertices() == cur_V);
            assert(cur_V > 1);
            
            std::copy(in_graph.address(0), in_graph.end(), valid.address(0));
            
            //maximum adjacency search from a
            update_heap<T> heap(V, (T)0, stack);
            
            assert (std::count(valid.address(0), valid.end(), true) == cur_V);
            
            int next_vertex_index = std::find(valid.address(0), valid.end(), true)-valid.address(0);
            
            assert (next_vertex_index < V);
            
            
            visit_vertex(valid, &heap, graph->get_row(next_vertex_index), graph->get_row(next_vertex_index+1), next_vertex_index);
            
            for (int j=1; j<cur_V-1; ++j) {

                assert (std::count(valid.address(0), valid.end(), true) == cur_V-j);
                
                next_vertex_index = heap.peek_max_index();
                
                assert (next_vertex_index < V);
                assert (next_vertex_index >= 0);
                assert (valid.get(next_vertex_index));
                
                visit_vertex(valid, &heap, graph->get_row(next_vertex_index), graph->get_row(next_vertex_index+1), next_vertex_index);
            }
            
            //there must be exactely 1 valid element left: (follows from loop invariant)
            assert (std::count(valid.address(0), valid.end(), true) == 1);

            //the next element that would be chosen defines a min-s-t cut
            int v_1 = heap.peek_max_index();
            
            assert (v_1 != next_vertex_index);
            assert (valid.get(v_1));
            
            //Store the cut of the phase
            T cut_of_the_phase = graph->sum_row(v_1);
            
            best_cut = std::min(best_cut, cut_of_the_phase);
            
            //contract edge
            assert(graph->number_of_vertices() == cur_V);
            
            graph->contract_edge(std::min(next_vertex_index, v_1), std::max(next_vertex_index, v_1), stack);
            in_graph.set(std::max(next_vertex_index, v_1), false);
            
            assert (i <= V-target_number_of_vertices);
            
            stack->leave(stack_record_inner);
        }
        
        assert(graph->number_of_vertices() == target_number_of_vertices);
        
        assert(std::count(in_graph.address(0), in_graph.end(), true) == 2);
        
        T cut_of_the_last_phase = graph->sum_row(std::find(in_graph.address(0), in_graph.end(), true)-in_graph.address(0));
        
        best_cut = std::min<T>(best_cut, cut_of_the_last_phase);
        
        
        stack->leave(stack_record);
        return best_cut;
    }
    
    
    
    template<class T>
    T deterministic_cut(sparse_graph<T, undirected_edge<T>> * u_graph, stack_allocator * stack) {
        stack_state stack_record = stack->enter();
        
        
        array<int> index = stack->allocate<int>(u_graph->number_of_vertices()+1);
        adjacency_array<T, undirected_edge<T>> adjacency_graph(u_graph, index);
        
        T cut = deterministic_cut_sparse<T>(&adjacency_graph, stack, 2);
        
        
        stack->leave(stack_record);
        return cut;
    }
    
    //uses an upper bound of the minimum cut to apply PR tests, preprocessing the graph in O(V^2/B) memtransfers
    template<class T>
    T deterministic_cut_pr(lazy_adjacency_matrix<T> * graph, stack_allocator * stack, T estimate) {
        stack_state stack_record = stack->enter();
        
        //PR1-preprocessing
        graph->pr_pass(estimate);
        
        assert (graph->number_of_vertices() > 1);
        
        if (graph->number_of_vertices() < graph->capacity()) {
            graph->compact(stack);
        }
        
        assert (graph->number_of_vertices() > 1);
        
        sparse_graph<T, undirected_edge<T>> sparse_graph(graph->adjacencies, stack, true);
        
        T cut = deterministic_cut(&sparse_graph, stack);
        
        stack->leave(stack_record);
        return cut;
    }
    
    //uses an upper bound of the minimum cut to apply PR tests, preprocessing the graph in O(V^2/B) memtransfers
    template<class T>
    T deterministic_cut_pr(indexed_adjacency_matrix<T> * indexed_graph, stack_allocator * stack, T estimate) {

        lazy_adjacency_matrix<T> * graph = &indexed_graph->matrix;

        return deterministic_cut_pr(graph, stack, estimate);
    }
    
    template<class T>
    T deterministic_cut_pr(sparse_graph<T, edge<T>> * graph, stack_allocator * stack, T estimate) {
        stack_state stack_record = stack->enter();
        
        
        assert (graph->number_of_vertices() > 1);
        
        graph->pr_pass(estimate, stack);
        
        graph->relabel_canonically(stack);
        
        //Convert to undirected representation (with edges going in both directions)
        sparse_graph<T, undirected_edge<T>> u_graph(graph->number_of_vertices(), 2*graph->number_of_edges(), stack);
        std::copy(graph->edges.address(0), graph->edges.end(), u_graph.edges.address(0));
        
        for (int i=0; i<graph->number_of_edges(); ++i) {
            u_graph.edges[graph->number_of_edges() + i].weight = graph->edges[i].weight;
            u_graph.edges[graph->number_of_edges() + i].set_vertices(graph->edges.get(i).vertex2(), graph->edges[i].vertex1());
        }
        
        T cut = deterministic_cut(&u_graph, stack);
        
        
        stack->leave(stack_record);
        return cut;
    }
}

#endif /* defined(____CO_MinCut_implementation_base____) */
