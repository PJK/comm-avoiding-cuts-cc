#ifndef GALOIS_LIGRAGRAPHCHIEXECUTOR_H
#define GALOIS_LIGRAGRAPHCHIEXECUTOR_H

#include "LigraExecutor.h"
#include "GraphChiExecutor.h"

namespace Galois {
//! Implementation of combination of Ligra and GraphChi DSL in Galois
namespace LigraGraphChi {

template<bool Forward,typename Graph,typename EdgeOperator,typename Bag>
void edgeMap(size_t size, Graph& graph, EdgeOperator op, Bag& output) {
  typedef Galois::Graph::BindSegmentGraph<Graph> WrappedGraph;
  WrappedGraph wgraph(graph);
  
  output.densify();
  Galois::GraphChi::hidden::vertexMap<false,false>(graph, wgraph,
      Galois::Ligra::hidden::DenseForwardOperator<WrappedGraph,Bag,EdgeOperator,Forward,true>(wgraph, output, output, op),
      static_cast<Bag*>(0),
      size);
}

template<bool Forward,typename Graph, typename EdgeOperator,typename Bag>
void edgeMap(size_t size, Graph& graph, EdgeOperator op, Bag& input, Bag& output, bool denseForward) {
  typedef Galois::Graph::BindSegmentGraph<Graph> WrappedGraph;
  WrappedGraph wgraph(graph);

  size_t count = input.getCount();

  if (!denseForward && count > graph.sizeEdges() / 20) {
    input.densify();
    if (denseForward) {
      abort(); // Never used now
      output.densify();
      Galois::GraphChi::hidden::vertexMap<false,false>(graph, wgraph,
        Galois::Ligra::hidden::DenseForwardOperator<WrappedGraph,Bag,EdgeOperator,Forward,false>(wgraph, input, output, op),
        static_cast<Bag*>(0),
        size);
    } else {
      Galois::GraphChi::hidden::vertexMap<false,false>(graph, wgraph,
        Galois::Ligra::hidden::DenseOperator<WrappedGraph,Bag,EdgeOperator,Forward>(wgraph, input, output, op),
        static_cast<Bag*>(0),
        size);
    }
  } else {
    Galois::GraphChi::hidden::vertexMap<true,false>(graph, wgraph,
      Galois::Ligra::hidden::SparseOperator<WrappedGraph,Bag,EdgeOperator,Forward>(wgraph, output, op),
      &input,
      size);
  }
}

template<bool Forward,typename Graph, typename EdgeOperator,typename Bag>
void edgeMap(size_t size, Graph& graph, EdgeOperator op, typename Graph::GraphNode single, Bag& output) {
  Bag input(graph.size());
  input.push(graph.idFromNode(single), 1);
  edgeMap<Forward>(size, graph, op, input, output, false);
}

template<typename... Args>
void outEdgeMap(Args&&... args) {
  edgeMap<true>(std::forward<Args>(args)...);
}

template<typename... Args>
void inEdgeMap(Args&&... args) {
  edgeMap<false>(std::forward<Args>(args)...);
}

template<bool UseGraphChi>
struct ChooseExecutor {
  template<typename... Args>
  void inEdgeMap(size_t size, Args&&... args) {
    edgeMap<false>(size, std::forward<Args>(args)...);
  }

  template<typename... Args>
  void outEdgeMap(size_t size, Args&&... args) {
    edgeMap<true>(size, std::forward<Args>(args)...);
  }

  template<typename Graph>
  void checkIfInMemoryGraph(Graph& g, size_t size) {
    if (Galois::GraphChi::hidden::fitsInMemory(g, size)) {
      g.keepInMemory();
    }
  }
};

template<>
struct ChooseExecutor<false> {
  template<typename... Args>
  void inEdgeMap(size_t size, Args&&... args) {
    Galois::Ligra::edgeMap<false>(std::forward<Args>(args)...);
  }

  template<typename... Args>
  void outEdgeMap(size_t size, Args&&... args) {
    Galois::Ligra::edgeMap<true>(std::forward<Args>(args)...);
  }

  template<typename Graph>
  void checkIfInMemoryGraph(Graph& g, size_t size) { }
};

} // end namespace
} // end namespace
#endif
