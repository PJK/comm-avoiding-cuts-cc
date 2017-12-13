#ifndef APPS_SSSP_GRAPHLABALGO_H
#define APPS_SSSP_GRAPHLABALGO_H

#include "Galois/DomainSpecificExecutors.h"
#include "Galois/Graph/OCGraph.h"
#include "Galois/Graph/LCGraph.h"
#include "Galois/Graph/GraphNodeBag.h"

#include <boost/mpl/if.hpp>

#include "SSSP.h"

struct GraphLabAlgo {
  typedef Galois::Graph::LC_CSR_Graph<SNode,uint32_t>
    ::with_no_lockable<true>::type
    ::with_numa_alloc<true>::type InnerGraph;
  typedef Galois::Graph::LC_InOut_Graph<InnerGraph> Graph;
  typedef Graph::GraphNode GNode;

  std::string name() const { return "GraphLab"; }

  void readGraph(Graph& graph) { readInOutGraph(graph); }

  struct Initialize {
    Graph& g;
    Initialize(Graph& g): g(g) { }
    void operator()(Graph::GraphNode n) {
      g.getData(n).dist = DIST_INFINITY;
    }
  };

  struct Program {
    Dist min_dist;
    bool changed;

    struct gather_type { };
    typedef int tt_needs_scatter_out_edges;

    struct message_type {
      Dist dist;
      message_type(Dist d = DIST_INFINITY): dist(d) { }

      message_type& operator+=(const message_type& other) {
        dist = std::min(dist, other.dist);
        return *this;
      }
    };

    void init(Graph& graph, GNode node, const message_type& msg) {
      min_dist = msg.dist;
    }

    void apply(Graph& graph, GNode node, const gather_type&) {
      changed = false;
      SNode& data = graph.getData(node, Galois::MethodFlag::NONE);
      if (data.dist > min_dist) {
        changed = true;
        data.dist = min_dist;
      }
    }

    bool needsScatter(Graph& graph, GNode node) {
      return changed;
    }
    
    void scatter(Graph& graph, GNode node, GNode src, GNode dst,
        Galois::GraphLab::Context<Graph,Program>& ctx, Graph::edge_data_reference edgeValue) {
      SNode& ddata = graph.getData(dst, Galois::MethodFlag::NONE);
      SNode& sdata = graph.getData(src, Galois::MethodFlag::NONE);
      Dist newDist = sdata.dist + edgeValue;
      if (ddata.dist > newDist) {
        ctx.push(dst, message_type(newDist));
      }
    }

    void gather(Graph& graph, GNode node, GNode src, GNode dst, gather_type&, Graph::edge_data_reference) { }
  };

  void operator()(Graph& graph, const GNode& source) {
    Galois::GraphLab::SyncEngine<Graph,Program> engine(graph, Program());
    engine.signal(source, Program::message_type(0));
    engine.execute();
  }
};

#endif
