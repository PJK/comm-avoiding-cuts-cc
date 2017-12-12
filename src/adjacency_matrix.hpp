//
//  adjacency_matrix.h
//  
//
//  Created by Lukas Gianinazzi on 18.03.15.
//
//

#ifndef _adjacency_matrix_h
#define _adjacency_matrix_h

#include <numeric>
#include <iostream>
#include "stack_allocator.h"

template <class T>
class adjacency_matrix_iterator;

//Square matrix.

template <class T>
class adjacency_matrix {
    
protected:
    
    array<T> elements;
    
public:
    
    //typedef adjacency_matrix_iterator<T> iterator_t;
    
    int vertex_count;
    
    int number_of_vertices() {
        return vertex_count;
    }
    
    T sum_row(int i) const {
        return std::accumulate<T*>(get_row(i), get_row(i+1), (T)0);
    }
    
    //no copy
    adjacency_matrix(adjacency_matrix<T> * graph) {
        set_elements(graph->vertex_count, graph->elements);
    }
    
    //allocates storage
    adjacency_matrix(long number_of_vertices, stack_allocator * storage) {
        set_elements(number_of_vertices, storage->allocate<T>(number_of_vertices*number_of_vertices));
    }
    
    adjacency_matrix(long number_of_vertices, T * storage) {
        array<T> elems(storage, number_of_vertices*number_of_vertices);
        set_elements(number_of_vertices, elems);
    }
    
    T * get_row(int u) const;
    array<T> get_row_array(int u) const;
    T get(int u, int v) const;
    
    array<T> get_elements() const {
        return elements;
    }
    
    void set_elements(long number_of_vertices, array<T> matrix);
    
    void compute_degrees(array<T> destination);
    
    bool is_symmetric() const;
    
    static size_t space_requirement(size_t capacity) {
        return capacity*capacity*sizeof(T);
    }
    
    friend std::ostream& operator<< (std::ostream &out, adjacency_matrix<T> &graph) {
        for (int i=0; i<graph.vertex_count; ++i) {
            for (int j = 0; j<graph.vertex_count; ++j) {
                out << graph.get(i, j) << " ";
            }
            out << std::endl;
        }
        return out;
    }

};

/*
template <class T>
class adjacency_matrix_iterator {

    adjacency_matrix<T> * graph;
    int row;
    int col;
    long m;
    long remaining_m;
public:
  
    adjacency_matrix_iterator(adjacency_matrix<T> * g) {
        graph = g;
        row = col = 0;
        m = 0;
        for (int i=0; i<number_of_vertices(); ++i) {
            for (int j=i+1; j<number_of_vertices(); ++j) {
                if (graph->get(i, j)) {
                    ++m;
                }
            }
        }
        remaining_m = m;
    }
  
    bool has_next() const {
        return remaining_m > 0;
    }
  
    void next() {
        
        assert (col<graph->number_of_vertices());
        assert (row<graph->number_of_vertices());
        
        while (graph->get(row, col) == 0) {
            
            assert (row<graph->number_of_vertices());
            
            ++col;
            
            if (col == graph->number_of_vertices()) {
                ++row;
                col = row+1;
            }
            
            assert (row<graph->number_of_vertices());
            assert (col<graph->number_of_vertices());
        }

        --remaining_m;
    }
  
    int vertex1() const {
        return row;
    }
  
    int vertex2() const {
        return col;
    }
  
    T value() const {
        return graph->get(row, col);
    }
  
    long number_of_edges() const {
        return m;
    }
  
    int number_of_vertices() const {
        return graph->number_of_vertices();
    }
  
};*/

template<class T>
T * adjacency_matrix<T>::get_row(int u) const  {
    assert (u>= 0);
    assert (u <= vertex_count);
    return elements.address(((long)vertex_count)*((long)u));
}

template<class T>
array<T> adjacency_matrix<T>::get_row_array(int u) const  {
    assert (u >= 0);
    assert (u < vertex_count);
    array<T> row(elements.address(((long)vertex_count)*((long)u)), vertex_count);
    return row;
}

template<class T>
T adjacency_matrix<T>::get(int u, int v) const  {
    assert (v >= 0);
    assert (u < vertex_count);
    assert (v < vertex_count);
    return get_row(u)[v];
}

template<class T>
void adjacency_matrix<T>::set_elements(long number_of_vertices, array<T> matrix) {
    assert (matrix.get_length() == (size_t) number_of_vertices*number_of_vertices);
    vertex_count = number_of_vertices;
    elements = matrix;
}

template<class T>
bool adjacency_matrix<T>::is_symmetric() const {
    
    for (int i=0; i<vertex_count; i++) {
        for (int j=i; j<vertex_count; j++) {
            if (elements[i*vertex_count+j] != elements[j*vertex_count+i]) {
                return false;
            }
        }
    }
    return true;
}

template<class T>
void adjacency_matrix<T>::compute_degrees(array<T> destination) {
    int length = vertex_count;
    for(int i=0; i<length; i++) {
        destination.set(i, std::accumulate<T*>(get_row(i), get_row(i+1), (T)0));
    }
}

#endif
