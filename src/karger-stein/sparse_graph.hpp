//
//  sparse_graph.hpp
//  
//
//  Created by Lukas Gianinazzi on 11.06.15.
//
//
//  Sparse graph representation using unsorted array of edges. Uses O(E) space for E edges, even if the number of vertices is larger than E

#ifndef _sparse_graph_hpp
#define _sparse_graph_hpp

#include <iostream>
#include "test/testing_utility.h"
#include "adjacency_matrix.hpp"
#include "stack_allocator.h"
#include <algorithm>

////
//VERTEX PAIR
////

class vertex_pair {
public:

    int vertices [2];
    
    int vertex1() const {
        return vertices[0];
    }
    
    int vertex2() const {
        return vertices[1];
    }
    
    void set_vertex1(int vertex) {
        vertices[0] = vertex;
    }
    
    void set_vertex2(int vertex) {
        vertices[1] = vertex;
    }
    
    void set_vertices(int vertex1, int vertex2) {
        vertices[0] = vertex1;
        vertices[1] = vertex2;
    }
    
    void set_vertices(vertex_pair edge) {
        vertices[0] = edge.vertices[0];
        vertices[1] = edge.vertices[1];
    }
    
    int value() const {
        return 1;
    }
    
    bool invariant() {
        return true;
    }
    
    void merge(vertex_pair * other) {
    }
    
    static bool compare_vertex1(vertex_pair edge1, vertex_pair edge2) {
        return edge1.vertex1() < edge2.vertex1();
    }
    
    static bool compare_vertex2(vertex_pair edge1, vertex_pair edge2) {
        return edge1.vertex2() < edge2.vertex2();
    }
    
    static bool compare_endpoints(vertex_pair edge1, vertex_pair edge2) {
        return edge1 < edge2;
    }
    
    static bool compare_endpoints_reverse(vertex_pair edge1, vertex_pair edge2) {
        return edge1.vertex2() < edge2.vertex2() || (edge1.vertex2() == edge2.vertex2() && edge1.vertex1() < edge2.vertex1());
    }
    
    bool operator<(vertex_pair edge2) const {
        return  vertex1() < edge2.vertex1() || (vertex1() == edge2.vertex1() && vertex2() < edge2.vertex2());
    }
    
    friend std::ostream& operator<< (std::ostream &out, vertex_pair &edge) {
        out << "(" << edge.vertex1() << ", " << edge.vertex2() << ")";
        return out;
    }
    
    void update() {
    }
    
};

////
//UNDIRECTED EDGE
////

template <class T>
class undirected_edge : public vertex_pair {
public:
    
    T weight;
    
    void merge(undirected_edge<T> * other) {
        this->weight += other->weight;
    }
    
    T value () {
        return weight;
    }
    
    friend std::ostream& operator<< (std::ostream &out, undirected_edge<T> &edge) {
        out << "(" << edge.vertex1() << ", " << edge.vertex2() << ", " << edge.weight << ")";
        return out;
    }
    
};

////
//EDGE
////

template <class T>
class edge : public undirected_edge<T> {
public:
    
    void update() {
        if (vertex_pair::vertices[1] < vertex_pair::vertices[0]) {
            std::swap<int>(vertex_pair::vertices[0], vertex_pair::vertices[1]);
        }
    }
    
    bool invariant() {
        return vertex_pair::vertex1() < vertex_pair::vertex2() && undirected_edge<T>::value() >= (T)0;
    }
    
};

////
//SCORED EDGE
////

class scored_edge : public vertex_pair {
public:
    
    float score;
    
    float value () {
        return score;
    }
    
    void merge(scored_edge * other) {
        this->score = std::min(score, other->score);
    }
    
    void update() {
        if (vertices[1] < vertices[0]) {
            std::swap<int>(vertices[0], vertices[1]);
        }
    }
    
    bool invariant() {
        return vertex1() < vertex2();
    }
    
    friend std::ostream& operator<< (std::ostream &out, scored_edge &edge) {
        out << "(" << edge.vertex1() << ", " << edge.vertex2() << ", " << edge.score << ")";
        return out;
    }
    
    static bool compare_score(scored_edge edge1, scored_edge edge2) {
        return edge1.score < edge2.score;
    }
    
};

////
//SPARSE GRAPH ITERATOR
////


template <class T, class EdgeT>
class sparse_graph;

template <class T, class EdgeT>
class sparse_graph_iterator {

  sparse_graph<T, EdgeT> * graph;
  long cur;

public:
  
  sparse_graph_iterator(sparse_graph<T, EdgeT> * g) {
    graph = g;
    cur = 0;
  }
  
  bool has_next() const {
    return graph->number_of_edges()>cur;
  }
  
  void next() {
    ++cur;
  }
  
  int vertex1() const {
    return graph->edges[cur].vertex1();
  }
  
  int vertex2() const {
    return graph->edges[cur].vertex2();
  }
  
  T value() const {
    return graph->edges[cur].value();
  }
  
  long number_of_edges() const {
    return graph->number_of_edges();
  }
  
  int number_of_vertices() const {
    return graph->number_of_vertices();
  }
  
};


////
//SPARSE GRAPH -- HEADER
////

template <class T, class EdgeT>
class sparse_graph {
    
    template <class S, class EdgeS>
    friend class sparse_graph;
    
    template <class S, class EdgeS>
    friend class adjacency_array;
    
private:
    
    bool invariant();
    
    int connected_components_sparse(array<vertex_pair> * labels, stack_allocator * stack);
    
    void compute_vertices(array<int> storage);
    
    static void inline apply_relabeling(array<EdgeT> edges, array<vertex_pair> relabeling, bool modify_first_vertex);
    
    int vertex_count = 0;
    
public:
    
    typedef sparse_graph_iterator<T, EdgeT> iterator_t;
    
    array<EdgeT> edges;//the edges in the graph
    
    sparse_graph();
    sparse_graph(int vertices, long long edges, stack_allocator  * edge_storage);
    sparse_graph(int vertices, array<EdgeT> edges);
    sparse_graph(sparse_graph<T, EdgeT> * prototype, stack_allocator  * storage);
    sparse_graph(sparse_graph<T, EdgeT> prototype, stack_allocator  * storage);
    sparse_graph(adjacency_matrix<T> matrix, stack_allocator * storage, bool count_edges_double);//count edges double indicates that both (i, j) and (j, i) should be added to the graph, otherwise only one of them is kept
    
    sparse_graph<T, EdgeT> prefix_graph(int prefix_number_of_edges);//returns the graph with only the first k edges (the result shares the same memory)
    sparse_graph<T, EdgeT> suffix_graph(int drop_number_of_edges);//returns the graph that does not contain the first k edges (the result shares the same memory)
    

    int connected_components(array<vertex_pair> * labels, stack_allocator * auxiliary_storage);//Returns the number of connected components of the graph. The labels array must be able to hold as many entries the graph has vertices. Postcondition is that the labels will contain a relabeling which identifies the connected components, that is for every connected component with at least 2 vertices, a single representative x is chosen and for every other vertex u in that component, (u,x) is in the array of labels (at an arbitrary position)
    
    void pr_pass(T upper_bound, stack_allocator * auxiliary_storage);//heuristic that contracts edges that are at least as heavy as the upper bound
    
    void relabel(array<vertex_pair> labels);//for every (u, v) in labels, merges vertex u into vertex v (that is, contracting {u,v}). There is a precondition that a vertex appearing as a second component in some vertex_pair cannot appear as a first component in another vertex_pair. This condition is satisfied by the labels returned from the connected_components method.
    
    void relabel_canonically(stack_allocator * auxiliary_storage);//renames the vertices so they go from 0 ... n-1
    
    void compact();//removes loops (edges connecting a vertex to itself) and parrallel edges

    friend std::ostream& operator<< (std::ostream &out, sparse_graph<T, EdgeT> &graph) {
        out << "graph with " << graph.number_of_vertices() << " vertices, edges:" <<  std::endl <<"{";
        for (long i=0; i<graph.number_of_edges(); ++i) {
            out << graph.edges[i];
            if (i < graph.number_of_edges()-1) {
                out << " ";
            }
        }
        out << "}" << std::endl;
        return out;
    }
    
    int number_of_vertices() {
        return vertex_count;
    }
    
    long number_of_edges() {
        return edges.get_length();
    }
    
    adjacency_matrix<T> as_dense_graph(stack_allocator * storage);
    
};


////
//SPARSE GRAPH -- CONSTRUCTORS
///

template <class T, class EdgeT>
sparse_graph<T, EdgeT>::sparse_graph(int vertices, long long edges, stack_allocator * edge_storage) {
    this->vertex_count = vertices;
    this->edges = edge_storage->allocate<EdgeT>(edges);
}

template <class T, class EdgeT>
sparse_graph<T, EdgeT>::sparse_graph(int vertices, array<EdgeT> edges) {
    this->vertex_count = vertices;
    this->edges = edges;
}

template <class T, class EdgeT>
sparse_graph<T, EdgeT>::sparse_graph(sparse_graph<T, EdgeT> * prototype, stack_allocator * storage) {
    this->vertex_count = prototype->number_of_vertices();
    this->edges = storage->allocate<EdgeT>(prototype->number_of_edges());
    std::copy(prototype->edges.begin(), prototype->edges.end(), this->edges.begin());
    
}

template <class T, class EdgeT>
sparse_graph<T, EdgeT>::sparse_graph(sparse_graph<T, EdgeT> prototype, stack_allocator * storage) : sparse_graph(&prototype, storage) {
}

template<class T, class EdgeT>
sparse_graph<T, EdgeT>::sparse_graph(adjacency_matrix<T> matrix, stack_allocator * edge_storage, bool count_edges_double) : sparse_graph(matrix.number_of_vertices(), 0, edge_storage) {

    long m = 0;
    
    for (int i=0; i < matrix.number_of_vertices(); ++i) {
        for (int j= count_edges_double ? 0 : i+1; j < matrix.number_of_vertices(); ++j) {
            if (matrix.get(i,  j) != 0) {
                ++m;
            }
        }
    }
    
    edges = edge_storage->allocate<EdgeT>(m);
    
    m = 0;
    
    for (int i=0; i < matrix.number_of_vertices(); ++i) {
        for (int j=count_edges_double ? 0 : i+1; j < matrix.number_of_vertices(); ++j) {
            if (matrix.get(i,  j) != 0) {
                edges[m].set_vertices(i, j);
                edges[m].weight = matrix.get(i, j);
                ++m;
            }
        }
    }
    
}


////
//SPARSE GRAPH -- IMPLEMENTATION (Public Methods)
////


template <class T, class EdgeT>
sparse_graph<T, EdgeT> sparse_graph<T, EdgeT>::prefix_graph(int keep_number_of_edges) {
    sparse_graph sub(number_of_vertices(), edges.prefix(keep_number_of_edges));
    return sub;
}

template <class T, class EdgeT>
sparse_graph<T, EdgeT> sparse_graph<T, EdgeT>::suffix_graph(int drop_number_of_edges) {
    sparse_graph sub(number_of_vertices(), edges.suffix(drop_number_of_edges));
    return sub;
}


template <class T, class EdgeT>
void sparse_graph<T, EdgeT>::relabel_canonically(stack_allocator * stack) {
    
    stack_state stack_record = stack->enter();
    
    int V = number_of_vertices();
    
    array<int> vertices = stack->allocate<int>(2*number_of_edges());
    compute_vertices(vertices);
    //test_print_array(vertices, num_vertices);
    
    array<vertex_pair> ranks = stack->allocate<vertex_pair>((long)V);
    
    int out = 0;
    for (int i=0; i<V; ++i) {
        if (vertices[i] != i) {
            ranks[out].set_vertices(vertices[i], i);
            ++out;
        }
    }
    
    //test_print_array(ranks, out-ranks);

    relabel(ranks.prefix(out));
    
    vertex_count = V;//necessary, as relabel assumes a star-shaped relabeling which is not the case here (TODO this is a bit of a hack)
    
    //std::cout << *this;
    
    assert(std::all_of(edges.begin(), edges.end(), [=] (EdgeT edge) {return edge.vertex1() < number_of_vertices() && edge.vertex2() < number_of_vertices();}));
    
    stack->leave(stack_record);
}

template <class T, class EdgeT>
adjacency_matrix<T> sparse_graph<T, EdgeT>::as_dense_graph(stack_allocator * storage) {
    
    //std::cout << *this;
    
    //PART 1 : Create graph with labels 0...number_of_vertices()-1
    relabel_canonically(storage);
    
    adjacency_matrix<T> matrix(number_of_vertices(), storage);
    std::fill<T*>(matrix.get_row(0), matrix.get_row(number_of_vertices()), (T)0);
    
    std::sort<EdgeT*>(edges.begin(), edges.end());
    
    //here, we should do sth like "rank"
    
    for (long i=0; i<number_of_edges(); ++i) {
        assert (edges[i].vertex2() < number_of_vertices());
        matrix.get_row(edges[i].vertex1())[edges[i].vertex2()] = edges[i].value();
    }
    
    std::sort<EdgeT*>(edges.begin(), edges.end(), EdgeT::compare_endpoints_reverse);
    
    for (long i=0; i<number_of_edges(); ++i) {
        assert (edges[i].vertex1() < number_of_vertices());
        matrix.get_row(edges[i].vertex2())[edges[i].vertex1()] = edges[i].value();
    }
    
    //std::cout << matrix;
    
    assert(matrix.is_symmetric());
    
    return matrix;
}

template <class T, class EdgeT>
void sparse_graph<T, EdgeT>::relabel(array<vertex_pair> labels) {

    long number_of_labels = labels.get_length();
    
    if (number_of_labels == 0) return;
    
    //std::cout << "relabel " << std::endl;
    //test_print_array(labels, number_of_labels);
    
    assert ( std::all_of (labels.begin(), labels.end(),
                       [] (vertex_pair renaming ) {
                           return renaming.vertex1() != renaming.vertex2();
                       }
                       ));

    assert (std::is_sorted(labels.begin(), labels.end(), vertex_pair::compare_vertex1));

    //Rename first vertex
    std::sort(edges.begin(), edges.end(), EdgeT::compare_vertex1);
    sparse_graph<T, EdgeT>::apply_relabeling(edges, labels, true);
    
    //Rename second vertex
    std::sort(edges.begin(), edges.end(), EdgeT::compare_vertex2);
    sparse_graph<T, EdgeT>::apply_relabeling(edges, labels, false);
    
    //Some edge types maintain an invariant (for example vertex1<vertex2), which we reestablish here
    for (long edge_index = 0; edge_index < number_of_edges(); ++edge_index) {
        edges[edge_index].update();
    }
    
    vertex_count = vertex_count - number_of_labels;//Note this is only true for star-shaped relabelings
    
    //remove parallel edges, loops
    compact();
}

template <class T, class EdgeT>
int sparse_graph<T, EdgeT>::connected_components(array<vertex_pair> * labels, stack_allocator * stack) {

    //std::cout<<"CC begin" << std::endl;

    int cc = connected_components_sparse(labels, stack);
    //test_printArray(labels_out, number_of_vertices());
    
    //std::cout<< "CC found " << *number_of_components_out << " components " << std::endl;
    
    assert(invariant());
    
    return cc;
}

template <class T, class EdgeT>
void sparse_graph<T, EdgeT>::compact() {
    //sort by (vertex1, vertex2)
    std::sort(edges.begin(), edges.end());
    //std::cout << *this;
    
    //scan and sum parallel edges, remove loops
    long m = 0;
    
    long dest = 0;
    long cur = 0;
    
    while (cur < number_of_edges()) {
        
        assert (dest <= number_of_edges());
        assert (dest <= cur);
        
        if (edges[cur].vertex1() != edges[cur].vertex2()) {//ignore loops
            
            ++m;
            edges.set(dest, edges[cur]);
            
            ++cur;
            
            //combine parallel edges
            while (cur < number_of_edges() && edges[cur].vertex1() == edges[dest].vertex1() && edges[cur].vertex2() == edges[dest].vertex2()) {
                assert (dest < cur);
                edges[dest].merge(edges.address(cur));
                ++cur;
            }
            
            ++dest;
        } else {
            ++cur;
        }
    }
    
    edges = edges.prefix(m);
    
    //std::cout << *this;
    
    assert(invariant());
}


//Heuristic that contracts edges heavier than upper_bound

template <class T, class EdgeT>
void sparse_graph<T, EdgeT>::pr_pass(T upper_bound, stack_allocator * auxiliary_storage) {
    
    stack_state stack_record = auxiliary_storage->enter();
    
    auto at_least_upper_bound = [upper_bound] (EdgeT edge) {
        return edge.value() >= upper_bound;
    };
    

    int initial_vertices = number_of_vertices();
    int remaining_vertices = initial_vertices;

    EdgeT * partition_point = std::partition(edges.begin(), edges.end(), at_least_upper_bound);
    
    //std::cout << "pr pass " << partition_point-edges << " out of " << number_of_edges << std::endl;
        
    while (partition_point - edges.begin() > 1) {
        
        sparse_graph<T, EdgeT> contractible_graph = prefix_graph(partition_point-edges.begin());
        
        array<vertex_pair> labels = auxiliary_storage->allocate<vertex_pair>(number_of_vertices());
        
        contractible_graph.connected_components(&labels, auxiliary_storage);
        
        remaining_vertices = number_of_vertices() - labels.get_length();
        
        if (remaining_vertices <= 2) {
            
            //break;
            partition_point = edges.address((long)((partition_point - edges.address(0))/1.1));
            
        } else {
            //Contract edges
            
            edges = edges.suffix(contractible_graph.number_of_edges());
            
            relabel(labels);
            
            break;
        }

    }
    
    /*if (remaining_vertices > 2 && remaining_vertices < initial_vertices / 2) {
        pr_pass(upper_bound, auxiliary_storage);
    }*/
    
    auxiliary_storage->leave(stack_record);
    
    assert (number_of_vertices() > 1);
    
    assert(invariant());
}


////
//SPARSE GRAPH -- IMPLEMENTATION (Private Methods)
////


//Algorithm due to Abello et al.
//Does not use search, but collectively operates on the graph to get good external memory performance
//Time is O(E log^2 E) for E edges - no matter how many vertices there are
template <class T, class EdgeT>
int sparse_graph<T, EdgeT>::connected_components_sparse(array<vertex_pair> * labels, stack_allocator * stack) {
    
    if (number_of_edges() <= 1) {
        if (number_of_edges() == 1) {
            labels->set(0, edges[0]);
        }
        *labels = labels->prefix(number_of_edges());
        return number_of_vertices() - number_of_edges();
    }
    
    stack_state stack_record = stack->enter();
    
    
    //recurse on first half
    array<vertex_pair> left_half_labels = *labels;
    
    sparse_graph<T, EdgeT> left_half = prefix_graph((number_of_edges()+1)/2);
    
    left_half.connected_components_sparse(&left_half_labels, stack);
    
    //relabel second half
    
    //left_half_cc =
    sparse_graph<T, EdgeT> right_half(suffix_graph(left_half.number_of_edges()), stack);
    
    right_half.relabel(left_half_labels);

    //assert(right_half.number_of_vertices() == left_half_cc);
    
    //recurse on second half
    
    array<vertex_pair> right_half_labels = stack->allocate<vertex_pair>(right_half.number_of_vertices());

    int right_half_cc = right_half.connected_components_sparse(&right_half_labels, stack);
    
    assert (right_half_cc <= right_half.number_of_vertices());


    //recover original labels from labels of contracted (second half) graph
    
    std::sort(left_half_labels.begin(), left_half_labels.end(), vertex_pair::compare_vertex2);
    
    sparse_graph<int, vertex_pair>::apply_relabeling(left_half_labels, right_half_labels, false);
    
    std::copy(right_half_labels.begin(), right_half_labels.end(), labels->address(left_half_labels.get_length()));
    
    *labels = labels->prefix(left_half_labels.get_length() + right_half_labels.get_length());
    
    sparse_graph<int, vertex_pair> labels_graph(right_half_cc, *labels);
    
    labels_graph.compact();
    
    *labels = labels_graph.edges;
    
    stack->leave(stack_record);
    
    return right_half_cc;
}


template <class T, class EdgeT>
void inline sparse_graph<T, EdgeT>::apply_relabeling(array<EdgeT> edges,  array<vertex_pair> labels, bool modify_first_vertex) {
    
    assert (std::is_sorted(labels.begin(), labels.end(), vertex_pair::compare_vertex1));
    assert (!modify_first_vertex || std::is_sorted(edges.begin(), edges.end(), vertex_pair::compare_vertex1));
    assert (modify_first_vertex || std::is_sorted(edges.begin(), edges.end(), vertex_pair::compare_vertex2));
    
    int number_of_labels = labels.get_length();
    int number_of_edges = edges.get_length();
    
    short access = modify_first_vertex ? 0 : 1;
    
    for (long edge_index = 0, label_index = 0; edge_index < number_of_edges && label_index < number_of_labels; ) {
        
        if (edges[edge_index].vertices[0+access] == labels[label_index].vertices[0]) {
            edges[edge_index].vertices[0+access] = labels[label_index].vertices[1];//relabel
            ++edge_index;
        } else if (edges[edge_index].vertices[0+access] < labels[label_index].vertices[0]) {
            ++edge_index;//vertex is not relabeled
        } else {
            ++label_index;//proceed to next label
        }
    }
    
}


template <class T, class EdgeT>
void sparse_graph<T, EdgeT>::compute_vertices(array<int> vertices) {

    int cur = 0;
    for (int i=0; i<number_of_edges(); ++i) {
        vertices.set(cur++, edges[i].vertex1());
        vertices.set(cur++, edges[i].vertex2());
    }
    
    std::sort(vertices.begin(), vertices.address(cur));
    std::unique(vertices.begin(), vertices.address(cur));
    
}


template <class T, class EdgeT>
bool sparse_graph<T, EdgeT>::invariant() {
    bool result = true;
    for (int edge = 0; edge < number_of_edges(); ++edge) {
        result = result && edges[edge].invariant();
        
        if (!result) {
            std::cout << " violating edge " << edges[edge] << " in " << std::endl;
            std::cout << *this;
        }
        
        assert(result);
    }
    return result;
}


#endif
