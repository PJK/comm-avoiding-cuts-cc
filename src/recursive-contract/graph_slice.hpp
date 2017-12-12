//
//  graph_slice.hpp
//
//
//  Created by Lukas Gianinazzi on 17.10.15.
//
//

#ifndef graph_slice_h
#define graph_slice_h

#include <stdio.h>
#include <random>
#include <algorithm>
#include "sum_tree.hpp"
#include "prng_engine.hpp"
#include "mpi.h"
#include <functional>
#include "utils.hpp"
#include "MPICollector.hpp"

//Used to represent edges for transfer of sparse subgraphs
//We can't send classes over MPI, so we need plain old data
//The MPI datatype for this is MPI_2INT
typedef struct {
    int v1;
    int v2;
} edge_struct_t;


//Represents a subset of the rows of an adjacency matrix of a graph
template <typename T>
class graph_slice {

    //total number of rows, columns in the graph
    int number_of_vertices = 0;

    //the slice has this many rows
    int rows_per_node = 0;

    ////the first row of this slice corresponds to the rank-th vertex in the graph
    int rank = 0;

    int size = 0;

    //Data is stored in row-major order
    T* slice = NULL;
    
    bool padding_is_zero() const;

public:

    //Time: O(1)
    //construct a slice containing no data
    graph_slice(int vertices, int rows_per_slice, int size) : graph_slice(vertices, rows_per_slice, 0, size, NULL) {};

    //The rank is the rank of the slice within the graph, that is the first slice has rank 0
    //Time: O(1)

    graph_slice(int vertices, int rows_per_slice, int rank, int size, T * data);

    //Copies this slice, allocates separate memory for the new slice
    //Time: O(n k), where n is the size of the graph and k is the number of rows in this slice
    graph_slice<T> deep_copy();
    
    //Default constructor.
    //Time: O(1)
    graph_slice();

    //Assignment (and deallocation of left operand memory).
    //Time: O(1)
    graph_slice<T> & operator= (const graph_slice<T>& other);

    //Time: O(1)
    void set_number_of_vertices(int v);

    //Time: O(1)
    int get_number_of_vertices() const;

    //Time: O(1)
    int get_rows_per_slice() const;

    // Time:O(1)
    int get_size() const;


    //The rank is the rank of the slice within the graph, that is the first slice has rank 0
    //Time: O(1)
    int get_rank() const;

    //returns a pointer to the first entry of the first row in the slice
    //Time: O(1)
    T* begin() const;

    //returns a pointer to the first entry of the i-th row in the slice
    //Time: O(1)
    T* get_row(long i) const;

    //returns a pointer to the j-th entry int the i-th row in the slice
    //Time: O(1)
    T* get(long i, long j) const;

    //Deallocates slice's memory.
    //Time: O(1).
    void free_slice();

    //returns the sum of the capacities of every vertex whose row is included in this slice
    //Time: O(n k), where n is the size of the graph and k is the number of rows in this slice
    T accumulate() const;

    //Zeroes the entries of this slice which correspond to loops in the graph
    //Time: O(k), where k is the number of rows in this slice
    void remove_loops();

    //Creates a new slice by combining the columns of this slice as indicated by the relabeling
    void combine_cols_using_relabeling(graph_slice<T> * dest_graph, int * relabeling) const;

    //Creates a new slice by combining the columns of this slice as indicated by the relabeling
    void combine_cols_using_relabeling_t(graph_slice<T> * dest_graph, int * relabeling) const;
    
    //Prints the matrix to std::cout in row major order
    //The communicator given must be such that the ranks in the communicator correspond the ranks of the slices
    void print(MPI_Comm comm) const;
    
    //Asserts that the padding is zero
    bool invariant();

};


//Creates prefix sums for a slice of the graph, allowing to select random edges in logarithmic time per edge
template <typename T>
class graph_slice_index {

    int row_offset; //the first row of this slice corresponds to the row_offset-th vertex in the graph
    int number_of_rows;
    T * sums;
    sum_tree<T> * capacities;//index over the sum of each row
    sum_tree<T> ** edge_weights;//indices, each over one row

public:

    //Selects an edge in the slice with probability proportional to its weight
    //Time: O(log n), where n is the number of vertices in the graph
    void select_random_edge(edge_struct_t * result, sitmo::prng_engine * random_engine);

    ~graph_slice_index();

    //Time: O(n k), where n is the number of vertices in the graph and k is the number of rows in this slice
    graph_slice_index(graph_slice<T> * graph);


};



////
//IMPLEMENTATION :: graph_slice
////

template <typename T>
graph_slice<T>::graph_slice(int vertices, int rows_per_slice, int rank, int size, T * data) {
    number_of_vertices = vertices;
    this->size = size;
    rows_per_node = rows_per_slice;
    slice = data;
    this->rank = rank;
}

template <typename T>
graph_slice<T>::graph_slice() {
	number_of_vertices = 0;
	rows_per_node = 0;
	slice = nullptr;
	this->rank = 0;
    size = 0;
}

template <typename T>
bool graph_slice<T>::invariant() {
    assert (padding_is_zero());
    return padding_is_zero();
}

template <typename T>
graph_slice<T> graph_slice<T>::deep_copy() {
    
    assert (padding_is_zero());//disallow deep copy of inconsistent objects
    
    T * c_slice = new T[(long)size * (long)rows_per_node];

    assert(get_row(rows_per_node) - begin() == size * rows_per_node);

    DebugUtils::print(-1, [&](std::ostream & out) {
        out << "will copy stuff of length " << size * rows_per_node << " composed of " << size << " and " << rows_per_node;
    });
    std::copy(begin(), get_row(rows_per_node), c_slice);

    DebugUtils::print(-1, [&](std::ostream & out) {
        out << "done copying stuff of length " << size * rows_per_node;
    });
    
    graph_slice<T> c(number_of_vertices, rows_per_node, rank, size, c_slice);
    
    return c;
}

//performs a shallow copy of other into this
//see http://courses.cms.caltech.edu/cs11/material/cpp/donnie/cpp-ops.html for rules on = overloading
template <typename T>
graph_slice<T> & graph_slice<T>::operator= (const graph_slice<T>& other) {
    if (this != &other) {
        free_slice();

        number_of_vertices = other.get_number_of_vertices();
        rows_per_node = other.get_rows_per_slice();
        size = other.get_size();
        slice = other.slice;
        this->rank = other.rank;
    }
	return *this;
}

template <typename T>
void graph_slice<T>::free_slice() {
    if(slice != nullptr) {
		delete[] slice;
    }
	slice = nullptr;
}

template <typename T>
T graph_slice<T>::accumulate() const {
    assert(slice != NULL);
    assert (padding_is_zero());
    return std::accumulate(slice, slice+size*rows_per_node, T());
}

template <typename T>
T * graph_slice<T>::begin() const {
    return slice;
}


template <typename T>
T * graph_slice<T>::get_row(long i) const {
    assert(i >= 0);
    assert(i <= rows_per_node);
    assert(slice != NULL);
    return slice+i*size;
}

template <typename T>
T * graph_slice<T>::get(long i, long j) const {
    assert(i >= 0);
    assert(i <= rows_per_node);
    assert(j >= 0);
    assert(j <= size);
    assert(slice != NULL);
    return slice+i*size+j;
}

template <typename T>
void graph_slice<T>::remove_loops() {
    assert(slice != NULL);
    assert (padding_is_zero());
    
    for (int i=0; i<rows_per_node; ++i) {
        *get(i, i+rank*rows_per_node) = 0;
    }
    
    assert (padding_is_zero());
}

template <typename T>
int graph_slice<T>::get_number_of_vertices() const {
    return number_of_vertices;
}

template <typename T>
int graph_slice<T>::get_rank() const {
    return rank;
}

template <typename T>
int graph_slice<T>::get_size() const {
    return size;

}

template <typename T>
int graph_slice<T>::get_rows_per_slice() const {
    return rows_per_node;
}


template <typename T>
void graph_slice<T>::print(MPI_Comm comm) const {

    int p = size / get_rows_per_slice();

    for (int processor=0; processor<p; ++processor) {

        if (rank == processor) {

			std::cout << "\nP" << rank << ": " << std::endl;

            for (int row=0; row < get_rows_per_slice(); ++row) {
                for (int col=0; col < size; ++col) {
                    std::cout << *get(row, col) << " ";
                }
                std::cout << std::endl;
            }

        }

        MPI::Barrier(comm);
    }

    if (rank == 0) {
        std::cout << std::endl;
    }
}

template <typename T>
void graph_slice<T>::set_number_of_vertices(int v) {

    number_of_vertices = v;
}

template <typename T>
void graph_slice<T>::combine_cols_using_relabeling(graph_slice<T> * dest, int * relabeling) const {

    std::fill<T*>(dest->begin(), dest->get_row(get_rows_per_slice()), T());

    for (int col = 0; col < get_size(); ++col) {
        int destination_col = relabeling[col];

        //Sum column
        for (int row = 0; row < get_rows_per_slice(); ++row) {
            *dest->get(row, destination_col) += *get(row, col);
        }
    }
}

template <typename T>
void graph_slice<T>::combine_cols_using_relabeling_t(graph_slice<T> * dest, int * relabeling) const {
    
    std::fill<T*>(dest->begin(), dest->get_row(get_rows_per_slice()), T());
    
    long k = get_rows_per_slice();
    
    for (long col = 0; col < get_number_of_vertices(); ++col) {
        long destination_col = relabeling[col];
        
        std::transform<T*>(slice+col*k, slice+(col+1)*k, dest->begin()+destination_col*k, dest->begin()+destination_col*k, std::plus<T>());
        
        //Sum column in the transposed matrix
        //for (int row = 0; row < get_rows_per_slice(); ++row) {
            //*dest->get(destination_col, row) += *get(col, row);
            //*dest->get(destination_col % k, (destination_col / k) * k + row) += *get(col % k, (col / k) * k + row);
        //}
    }
    
}


template <typename T>
bool graph_slice<T>::padding_is_zero() const {
    bool inv = true;
    for (int i=0; i<get_rows_per_slice(); ++i) {
        bool padding_is_zero = !slice || std::all_of(get(i, get_number_of_vertices()), get_row(i+1), [=] (T val) {return val==0;});
        assert (padding_is_zero);
        inv = inv && padding_is_zero;
    }
    return inv;
}

////
//IMPLEMENTATION :: graph_slice_index
////


template <typename T>
void graph_slice_index<T>::select_random_edge(edge_struct_t * result, sitmo::prng_engine * random_engine) {

    std::uniform_int_distribution<long> uniform_row(1, capacities->root());

    result->v1 = capacities->lower_bound(uniform_row(*random_engine));

    std::uniform_int_distribution<long> uniform_column(1, edge_weights[result->v1]->root());

    result->v2 = edge_weights[result->v1]->lower_bound(uniform_column(*random_engine));

    result->v1 += row_offset;

    //std::cout << result->v1 << ", " << result->v2 << std::endl;
}

template <typename T>
graph_slice_index<T>::~graph_slice_index() {

    delete[] sums;
    delete capacities;
    for (int i=0; i<number_of_rows; ++i) {
        delete edge_weights[i];
    }
    delete[] edge_weights;
}

template <typename T>
//The rank is the rank of the slice within the graph, that is the first slice has rank 0
graph_slice_index<T>::graph_slice_index(graph_slice<T> * graph) {

    number_of_rows = graph->get_rows_per_slice();

    row_offset = graph->get_rank() * graph->get_rows_per_slice();

    edge_weights = new sum_tree<T>*[number_of_rows];

    sums = new T[number_of_rows];

    for (int i=0; i<number_of_rows; ++i) {
        edge_weights[i] = new sum_tree<T>(graph->get_row(i), graph->get_number_of_vertices());
        sums[i] = edge_weights[i]->root();
    }

    capacities = new sum_tree<T>(sums, number_of_rows);

}

#endif /* graph_slice_h */
