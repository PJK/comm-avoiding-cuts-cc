//
//  lazy_adjacency_matrix.hpp
//  
//
//  Created by Lukas Gianinazzi on 18.03.15.
//
//

#ifndef _lazy_adjacency_matrix_hpp
#define _lazy_adjacency_matrix_hpp

#include "bulk_union_find.h"
#include "adjacency_matrix.hpp"
#include "algorithm"
#include "matrices.hpp"
#include "assert.h"
#include <functional>   // std::plus

template <class T>
class lazy_adjacency_matrix {
    
    bulk_union_find union_find;
    
    void rank(array<int> source, array<int> destination);
    void sum_rows(T * src, T * dest, int n);
    void clean_rows();
public:
    
    lazy_adjacency_matrix(int number_of_vertices, stack_allocator * storage) :
     union_find(storage->allocate<int>(number_of_vertices)),
     adjacencies(number_of_vertices, storage)
    {
    }
    
    //initializes this matrix to a copy of the given graph
    lazy_adjacency_matrix(adjacency_matrix<T> * graph, stack_allocator * storage) : lazy_adjacency_matrix(graph->vertex_count, storage)
    {
        std::copy(graph->get_row(0), graph->get_row(graph->vertex_count), get_row(0));
    }
    
    //initializes this matrix to a copy of the given graph
    lazy_adjacency_matrix(lazy_adjacency_matrix<T> * graph, stack_allocator * storage) : lazy_adjacency_matrix(graph->capacity(), storage)
    {
        if (graph->capacity() < graph->number_of_vertices()) {
            std::copy<int*>(graph->union_find.rep.begin(), graph->union_find.rep.end(), union_find.rep.begin());
        }
        
        std::copy(graph->get_row(0), graph->get_row(graph->capacity()), get_row(0));
    }
    
    adjacency_matrix<T> adjacencies;
                
    int rep(int i) const;
    
    void contract_edge(int i, int j);
    
    void compact(stack_allocator * aux_storage);
    
    int number_of_vertices() const ;
    
    int capacity() const;
    
    T * get_row(int u) const;
    
    T sum_row(int i) const;
    
    static size_t space_requirement(size_t capacity) {
        return adjacency_matrix<T>::space_requirement(capacity) + sizeof(int)*capacity;
    }
    
    void compute_degrees(array<T> destination) {
        adjacencies.compute_degrees(destination);
    }
    
    void pr_pass(T estimate) {
        int cap = capacity();
        for (int i=0; i<cap; ++i) {
            if (rep(i) == i && number_of_vertices() > 2) {
                T * row = get_row(i);
                for (int j=0; j<cap; ++j) {
                    if (row[j] >= estimate) {
                        //++pr_counter;
                        contract_edge(i, rep(j));
                        break;
                    }
                }
            }
        }
    }
};

template <class T>
T * lazy_adjacency_matrix<T>::get_row(int u) const {
    return adjacencies.get_row(u);
}

template <class T>
int lazy_adjacency_matrix<T>::number_of_vertices() const {
    return union_find.number_of_sets;
}

template <class T>
int lazy_adjacency_matrix<T>::capacity() const {
    return adjacencies.vertex_count;
}

template <class T>
int lazy_adjacency_matrix<T>::rep(int i) const {
    return union_find.rep[i];
}


template <class T>
T lazy_adjacency_matrix<T>::sum_row(int j) const {
    return adjacencies.sum_row(j);
}


template <class T>
void lazy_adjacency_matrix<T>::contract_edge(int i, int j) {
    assert(i != j);
    
    //perform bulk union
    union_find.bulk_union(i, j);
    
    T * row_i = adjacencies.get_row(i);
    T * row_j = adjacencies.get_row(j);
    
    //test_printArray(union_find.rep, this->vertex_count);
    
 
    //scan row i, row j and union_find.rep at the same time
    int vertices = adjacencies.vertex_count;
    
    //std::cout << "proceed to add rows (" << vertices << ") vertices" << std::endl;
    
    //#pragma omp parallel for
    for (int k=0; k<vertices; k++) {
        
        //add row i to row j
        row_j[k] += row_i[k];
        //remove loops
        if (union_find.rep[k] == j) {
            row_j[k] = (T)0;
        }
        //this is actually not necessary
        //zero out row i
        //row_i[k] = (T)0;
    }
    
    //std::cout << "done with contracting" << std::endl;
    
}


//precondition: aux_storage needs to be able to hold union_find.number_of_sets * vertex_count element of type T and vertex_count elements of type int
template <class T>
void lazy_adjacency_matrix<T>::compact(stack_allocator * aux_storage) {
    //the element matrix (at element_storage) is of size vertex_count * vertex_count
    stack_state saved_stack_state = aux_storage->enter();
    
    long original_capacity = adjacencies.vertex_count;
    long target_capacity = union_find.number_of_sets;
    
    //remove invalid rows
    clean_rows();
    //the element matrix is now of size target_capacity * vertex_count
    
    //this will give the correspondance between the columns (= between the rows of the transpose)
    array<int> correspondence = aux_storage->allocate<int>(original_capacity);
    
    //transpose
    //the aux_matrix is of size vertex_count * target_capacity, as it is the transpose of the row-cleaned element matrix
    
    array<T> aux_matrix = aux_storage->allocate<T>(target_capacity*original_capacity);
    matrices::transpose<T>(adjacencies.get_row(0), aux_matrix.begin(), target_capacity, original_capacity);
    
    //the ranks of the representatives define the correspondance between the rows of the aux_matrix
    rank(union_find.rep, correspondence);
    
    //now we extend the ranks, such that indices contain the rank of their representative. This allows us to discard the rep array
    for (int i=0; i<original_capacity; i++) {
        correspondence.set(i, correspondence[union_find.rep[i]]);
    }
    
    //combine rows of transpose
    //the destination matrix is of size target_capacity * target_capacity
    
    adjacencies.set_elements(target_capacity, adjacencies.get_elements().prefix((long)target_capacity*target_capacity));
    
    //sum together corresponding rows
    array<bool> initialized = aux_storage->allocate<bool>(target_capacity);
    std::fill<bool*>(initialized.begin(), initialized.end(), false);
    
    for (long i=0; i<original_capacity; ++i) {
        T * src_row = aux_matrix.address(i*target_capacity);
        int dest_i = correspondence[i];
        
        T * dest_row = adjacencies.get_row(dest_i);
        
        if (initialized[dest_i]) {//not the first row
            sum_rows(src_row, dest_row, target_capacity);
        } else {//the first row
            std::copy(src_row, src_row+target_capacity, dest_row);
            initialized.set(dest_i, true);
        }
        
    }
    //the element matrix is now of size target_capacity * target_capacity
    
    //reinitialize smaller union find structure
    union_find.compact();
    assert(adjacencies.is_symmetric());
    
    aux_storage->leave(saved_stack_state);
}

//postcondition: destination[i] contains the rank of i in 'source' if i appears in source and an arbitrary value otherwise
template <class T>
void lazy_adjacency_matrix<T>::rank(array<int> source, array<int> destination) {
    
    
    int n = source.get_length();
    //mark all values that appear with a 1
    std::fill<int*>(destination.begin(), destination.end(), 0);
    for (int i=0; i<n; i++) {
        destination.set(source[i], 1);
    }
    //the partial sums almost give the rank (1 based so we need to subtract 1 to get 0-based ranks)
    destination.set(0, destination[0]-1);
    std::partial_sum<int*>(destination.begin(), destination.end(), destination.begin());
    
}

template <class T>
void lazy_adjacency_matrix<T>::sum_rows(T * src, T * dest, int n) {
    std::transform<T*>(src, src+n, dest, dest, std::plus<T>());
}

template <class T>
void lazy_adjacency_matrix<T>::clean_rows() {
    T * cur_dest = adjacencies.get_row(0);
    int vertices = adjacencies.vertex_count;
    for (int i=0; i<vertices; i++) {
        if (union_find.rep[i] == i) {//valid row
            cur_dest = std::copy<T*>(adjacencies.get_row(i), adjacencies.get_row(i+1), cur_dest);
        }
    }
}

#endif
