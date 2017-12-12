#include "SequentialKargerSteinCut.hpp"
#include "AdjacencyListGraph.hpp"



AdjacencyListGraph::Weight SequentialKargerSteinCut::compute() {
    // TODO can we make this nicer?
    // (Could rewrite KS using the new graph class and the iterated sampling we have for the sqrt cut)
    long m = graph_->edge_count();
    
    stack_allocator allocator(0);
    array<edge<AdjacencyListGraph::Weight>> ks_edges = allocator.allocate<edge<AdjacencyListGraph::Weight>>(m);
    
    for (unsigned i { 0 }; i < m; i++) {
        edge<AdjacencyListGraph::Weight> e;
        e.set_vertices(graph_->edges()[i].from, graph_->edges()[i].to);
        e.weight = graph_->edges()[i].weight;
        ks_edges[i] = e;
    }
    
    sparse_graph<AdjacencyListGraph::Weight, edge<AdjacencyListGraph::Weight>> ks_graph(graph_->vertex_count(), ks_edges);

    AdjacencyListGraph::Weight ks_min = comincut::minimum_cut(&ks_graph, success_probability_, seed_);
    

    return ks_min;
}
