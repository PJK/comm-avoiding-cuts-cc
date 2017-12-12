//
//  lazy_adjacency_matrix.hpp
//  
//
//  Created by Lukas Gianinazzi on 17.04.15.
//
//

#ifndef _indexed_adjacency_matrix_hpp
#define _indexed_adjacency_matrix_hpp

#include "bulk_union_find.h"
#include "adjacency_matrix.hpp"
#include "lazy_adjacency_matrix.hpp"
#include "algorithm"
#include "matrices.hpp"
#include "assert.h"
#include <functional>   // std::plus
#include <random>
#include "prng_engine.hpp"
#include "sum_tree.hpp"
#include "stack_allocator.h"

template <class T>
class indexed_adjacency_matrix {
    
    
    array<T> capacities;
    sum_tree<T> capacities_sum_tree;
    array<T> row_capacities_tree;//auxiliary storage to hold the sum_tree for the row currently being contracted
    void init_capacities();
    
    int random_select(sum_tree<T> * sum_tree, sitmo::prng_engine * random_engine);
    
    int select(array<T> capacities);
    
    bool invariant();
    
    void set_capacities(array<T> precomputed_capacities, stack_allocator * storage) {
        
        assert(precomputed_capacities.get_length() == (size_t) capacity());
        
        capacities = storage->allocate<T>(capacity());
        
        std::copy<T*>(precomputed_capacities.begin(), precomputed_capacities.end(), capacities.begin());
        
        capacities_sum_tree.init(capacities, storage->allocate<T>(sum_tree<T>::number_of_elements_required(capacity())));
        
        row_capacities_tree = storage->allocate<T>(sum_tree<T>::number_of_elements_required(capacity()));
    }
    
public:
    
    //copies the graph (TODO) no copy required
    indexed_adjacency_matrix(adjacency_matrix<T> * adjacencies, array<T> precomputed_capacities, stack_allocator * storage) : matrix(adjacencies, storage) {
        set_capacities(precomputed_capacities, storage);
        //matrix.adjacencies = *adjacencies;
        assert (invariant());
    }
    
    //copies the graph
    indexed_adjacency_matrix(indexed_adjacency_matrix<T> * source_graph, stack_allocator * storage) : matrix(&source_graph->matrix, storage) {
        set_capacities(source_graph->capacities, storage);
        assert (invariant());
    }
    
    lazy_adjacency_matrix<T> matrix;

    T contract_edge(int i, int j);
    
    T contract_random_edge(sitmo::prng_engine * random_engine);
    
    void compact(stack_allocator * aux_storage);

    int capacity() const;
    
    int number_of_vertices() const;
    
    
    void pr_pass(T estimate) {
        int cap = capacity();
        
        for (int i=0; i<cap && number_of_vertices() > 2; ++i) {
            if (capacities[i] >= estimate) {
                
                assert (matrix.rep(i) == i);
                T * row = matrix.get_row(i);
                for (int j=0; j<cap; ++j) {
                    if (row[j] >= estimate) {
                        //TODO cases if i<rep(j)  -> add row rep(j) to row i if rep(j)<i, otw. add row i to row rep(j) and break
                        contract_edge(i, matrix.rep(j));
                        break;
                    }
                }
                
            }
        }
    }
    
};


template <class T>
int indexed_adjacency_matrix<T>::capacity() const {
    return matrix.capacity();
}

template <class T>
int indexed_adjacency_matrix<T>::number_of_vertices() const {
    return matrix.number_of_vertices();
}

template<class T>
int indexed_adjacency_matrix<T>::random_select(sum_tree<T> * sum_tree, sitmo::prng_engine * random_engine) {
    T sum = sum_tree->root();
    
    std::uniform_int_distribution<T> uniform_int(1, sum);
    T r = uniform_int(*random_engine);

    int index = sum_tree->lower_bound(r);
    
    assert (index >= 0);
    assert (index < capacity());
    
    return index;
}

template<class T>
int indexed_adjacency_matrix<T>::select(array<T> capacities) {//select the first nonzero row
    int i=0;
    while (capacities[i] == 0) {
        ++i;
    }
    return i;
}

template <class T>
T indexed_adjacency_matrix<T>::contract_edge(int i, int j) {
    
    matrix.contract_edge(i, j);

    T new_capacity = matrix.sum_row(j);
    
    T contracted_weight = (capacities[j]+capacities[i]-new_capacity)/2;
    capacities_sum_tree.update(j, new_capacity);
    capacities_sum_tree.update(i, (T)0);
    
    return contracted_weight;
}

template <class T>
T indexed_adjacency_matrix<T>::contract_random_edge(sitmo::prng_engine * random_engine) {

    int i = random_select(&capacities_sum_tree, random_engine);
    
    assert (matrix.rep(i) == i);
    assert(i<capacity());
    
    array<T> row = matrix.adjacencies.get_row_array(i);
    sum_tree<T> sums(row, row_capacities_tree);
    
    int j = random_select (&sums, random_engine);

    j = matrix.rep(j);
    
    T contracted_weight = contract_edge(std::max(i, j), std::min(i, j));
    
    assert (invariant());
    
    return contracted_weight;
}

//precondition: aux_storage needs to be able to hold union_find.number_of_sets * vertex_count element of type T and vertex_count elements of type int
template <class T>
void indexed_adjacency_matrix<T>::compact(stack_allocator * aux_storage) {
    
    matrix.compact(aux_storage);
    
    for (int i=0, j=0; j<capacity(); ++i) {
        if (capacities[i]) {
            capacities.set(j , capacities[i]);
            ++j;
        }
    }
    
    capacities = capacities.prefix(capacity());
    
    capacities_sum_tree.init(capacities, capacities_sum_tree.get_tree());
    
    assert (invariant());
}

template <class T>
bool indexed_adjacency_matrix<T>::invariant() {
    
    assert (capacities.get_length() == (size_t) capacity());
    
    bool result = true;
    
    for (int i=0; i<capacity(); ++i) {
        
        if (matrix.rep(i) == i) {
            
            for (int j=0; j<capacity(); ++j) {
                assert (capacities[i] >= matrix.get_row(i)[j]);//simple test for overflows
            }
            
            T actual_sum = matrix.sum_row(i);
            
            assert (actual_sum == capacities[i]);
            result = result && actual_sum == capacities[i];
        }
    }
    
    return result;
}



#endif
