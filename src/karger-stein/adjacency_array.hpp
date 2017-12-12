//
//  adjacency_array.hpp
//  
//
//  Created by Lukas Gianinazzi on 22.06.15.
//
//

#ifndef _adjacency_array_hpp
#define _adjacency_array_hpp

#include "sparse_graph.hpp"
#include "stack_allocator.h"

////
//ADJACENCY ARRAY -- HEADER
////

template <class T, class EdgeT>
class adjacency_array {
    
    sparse_graph<T, EdgeT> * graph;
    array<int> index;
    void compute_capacities(array<T> destination);
    
public:
    
    int index_length;
    
    int number_of_vertices();
    
    long number_of_edges() const;
    
    adjacency_array(sparse_graph<T, EdgeT> * unordered_graph, array<int> index_storage);
    
    void compute_index();
    

    void contract_edge(int u, int v, stack_allocator * auxiliary_storage);
    
    EdgeT * get_row(int i) const;
    
    T sum_row(int i) const;
    
    T pr_pass(T upper_bound, stack_allocator * auxiliary_storage);
    
    friend std::ostream& operator<< (std::ostream &out, adjacency_array<T, EdgeT> &graph) {
        out << *(graph.graph);
        
        for (int i=0; i<graph.index_length; ++i) {
            out << graph.index[i];
        }
        
        return out;
    }
};


////
//ADJACENCY ARRAY -- IMPLEMENTATION (Constructors)
////

template <class T, class EdgeT>
adjacency_array<T, EdgeT>::adjacency_array(sparse_graph<T, EdgeT> * unordered_graph, array<int> index_storage) {
    this->graph = unordered_graph;
    this->index = index_storage;
    this->index_length = unordered_graph->number_of_vertices();
    
    std::sort(graph->edges.address(0), graph->edges.address(graph->number_of_edges()));
    compute_index();
}

////
//ADJACENCY ARRAY -- IMPLEMENTATION (Public Methods)
////

template <class T, class EdgeT>
long adjacency_array<T, EdgeT>::number_of_edges() const {
    return graph->number_of_edges();
}

template <class T, class EdgeT>
T adjacency_array<T, EdgeT>::pr_pass(T upper_bound, stack_allocator * auxiliary_storage) {
    /*
    bool * scanned = (bool*)auxiliary_storage;
    std::fill(scanned, scanned+index_length, false);
    T * capacities = (T*)(scanned + index_length);
    std::fill(capacities, capacities+index_length, (T)0);
    compute_capacities(capacities);
    
    sparse_graph<int, vertex_pair> contractible_graph(0, 0, (vertex_pair*)(capacities+index_length));
    
    for (int i=0; i<index_length; ++i) {
        pr_test34(i, scanned, capacities, contractible_graph);
    }
    */
    
}

template <class T, class EdgeT>
void adjacency_array<T, EdgeT>::compute_capacities(array<T> destination) {
    
    for (int i=0; i<number_of_edges(); ++i) {
        destination[graph->edges[i].vertex1()] += graph->edges[i].weight;
    }
    
}

template <class T, class EdgeT>
int adjacency_array<T, EdgeT>::number_of_vertices() {
    return graph->number_of_vertices();
}

template <class T, class EdgeT>
void adjacency_array<T, EdgeT>::compute_index() {

    assert(std::is_sorted(graph->edges.address(0), graph->edges.address(graph->number_of_edges())));
    
    //get the index of the first edge for every vertex
    for (long i=0, current_edge = 0; i < index_length; ++i) {
        index[i] = current_edge;
        
        while (current_edge < graph->number_of_edges() && graph->edges[current_edge].vertex1() == i) {
            ++current_edge;
        }
    }
    index[index_length] = graph->number_of_edges();//sentinel value
    
    assert(std::is_sorted(graph->edges.address(0), graph->edges.address(graph->number_of_edges())));
}


template <class T, class EdgeT>
void adjacency_array<T, EdgeT>::contract_edge(int u, int v, stack_allocator * auxiliary_storage) {
    
    stack_state stack_record = auxiliary_storage->enter();
    
    assert (u < v);
    assert (0 <= u && u < index_length);
    assert (0 <= v && v < index_length);
    
    assert(std::is_sorted(graph->edges.address(0), graph->edges.address(graph->number_of_edges())));
    
    array<EdgeT> contracted_graph = auxiliary_storage->allocate<EdgeT>(graph->number_of_edges());
    
    EdgeT * first_destination = (EdgeT*)contracted_graph.address(0);
    EdgeT * destination = (EdgeT*)contracted_graph.address(0);
    EdgeT * source = graph->edges.address(0);
    EdgeT * last_u = NULL;
    
    while (source < graph->edges.address(graph->number_of_edges())) {
        

        if (source->vertex1() != u && source->vertex1() != v) {
            
            if (source->vertex2() != v) {
                *destination = *source;
                if (source->vertex2() == u) {
                    last_u = destination;
                }
                ++destination;
                
            } else if (source->vertex2() == v) {
                
                if (last_u != NULL && last_u->vertex1() == source->vertex1()) {
                    //There is a "u" for the same vertex
                    last_u->merge(source);
                    
                } else {
                    
                    //there is no "u" --> need to insert
                    //TODO this is probably more elegant using swap
                    EdgeT * insertion_pt = destination;
                    while (insertion_pt>first_destination && (insertion_pt-1)->vertex1() == source->vertex1() && (insertion_pt-1)->vertex2() > u) {
                        --insertion_pt;
                    }
                    
                    std::copy_backward(insertion_pt, destination, destination+1);
                    
                    assert (std::is_sorted(insertion_pt+1, destination+1));
                    
                    
                    *insertion_pt = *source;
                    insertion_pt->set_vertex2(u);
                    ++destination;
                    
                }
                
            }
            
            ++source;
            

            
        } else if (source->vertex1() == u) {
            //merge
            assert (source - graph->edges.address(0) == index[u]);
            
            EdgeT * cur_u = graph->edges.address(index[u]);
            EdgeT * last_u = graph->edges.address(index[u+1]);
            
            EdgeT * cur_v = graph->edges.address(index[v]);
            EdgeT * last_v = graph->edges.address(index[v+1]);
            
            while (cur_u < last_u && cur_v < last_v) {
                //Remove loops
                if (cur_u->vertex2() == v) {
                    ++cur_u;
                } else if (cur_v->vertex2() == u) {
                    ++cur_v;
                } else if (cur_u->vertex2() < cur_v->vertex2()) {
                    *destination = *cur_u;
                    
                    assert(*destination < *cur_v);
                    
                    ++destination;
                    ++cur_u;
                } else if (cur_u->vertex2() > cur_v->vertex2()) {
                    *destination = *cur_v;
                    destination->set_vertex1(u);
                    
                    assert(*destination < *cur_u);
                    
                    ++destination;
                    ++cur_v;
                } else {
                    assert (cur_u->vertex2() == cur_v->vertex2());
                    
                    *destination = *cur_u;
                    destination->merge(cur_v);
                    
                    ++destination;
                    ++cur_u;
                    ++cur_v;
                }
                assert (cur_v <= last_v && cur_u <= last_u);
            }
            assert (cur_v == last_v || cur_u == last_u);
            
            //Finish up
            while (cur_u < last_u) {
                assert (cur_v == last_v);
                //Remove loops
                if (cur_u->vertex2() != v) {
                    *destination = *cur_u;
                    ++destination;
                }
                ++cur_u;
            }
            while (cur_v < last_v) {
                assert (cur_u == last_u);
                //Remove loops
                if (cur_v->vertex2() != u) {
                    *destination = *cur_v;
                    destination->set_vertex1(u);
                    ++destination;
                }
                ++cur_v;
            }
 
            source = last_u;
            
        } else {
            assert (source->vertex1() == v);
            //SKIP (we already merged these edges when we met u)
            ++source;
            //source = graph->edges+index[v+1];
        }
        
        assert(std::is_sorted(first_destination, destination));
    }
    
    assert (std::all_of(first_destination, destination, [=] (EdgeT edge) {return edge.vertex1() != v && edge.vertex2() != v;} ));
    
    std::copy (first_destination, destination, graph->edges.address(0));
    
    assert (destination-first_destination <= graph->number_of_edges());
    
    graph->edges = graph->edges.prefix(destination-first_destination);
    --graph->vertex_count;
    
    compute_index();
    
    auxiliary_storage->leave(stack_record);
}


template <class T, class EdgeT>
EdgeT * adjacency_array<T, EdgeT>::get_row(int i) const {
    assert (i<=index_length);
    return graph->edges.address(index[i]);
}


template <class T, class EdgeT>
T adjacency_array<T, EdgeT>::sum_row(int i) const {
    T sum = (T)0;
    for (EdgeT * edge = get_row(i); edge < get_row(i+1); ++edge) {
        sum += edge->value();
    }
    return sum;
}


#endif
