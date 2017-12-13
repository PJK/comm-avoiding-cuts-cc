/** GMetis -*- C++ -*-
 * @file
 * @section License
 *
 * Galois, a framework to exploit amorphous data-parallelism in irregular
 * programs.
 *
 * Copyright (C) 2013, The University of Texas at Austin. All rights reserved.
 * UNIVERSITY EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES CONCERNING THIS
 * SOFTWARE AND DOCUMENTATION, INCLUDING ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR ANY PARTICULAR PURPOSE, NON-INFRINGEMENT AND WARRANTIES OF
 * PERFORMANCE, AND ANY WARRANTY THAT MIGHT OTHERWISE ARISE FROM COURSE OF
 * DEALING OR USAGE OF TRADE.  NO WARRANTY IS EITHER EXPRESS OR IMPLIED WITH
 * RESPECT TO THE USE OF THE SOFTWARE OR DOCUMENTATION. Under no circumstances
 * shall University be liable for incidental, special, indirect, direct or
 * consequential damages or loss of profits, interruption of business, or
 * related expenses which may arise from use of Software or Documentation,
 * including but not limited to those resulting from defects in Software and/or
 * Documentation, or loss or inaccuracy of data of any kind.
 *
 * @author Xin Sui <xinsui@cs.utexas.edu>
 * @author Nikunj Yadav <nikunj@cs.utexas.edu>
 * @author Andrew Lenharth <andrew@lenharth.org>
 */


#include "Metis.h"
#include "Galois/Galois.h"
#include "Galois/Accumulator.h"
#include "Galois/Runtime/PerThreadStorage.h"

#include <iostream>

namespace {

void assertAllMatched(GNode node, GGraph* graph) {
  for (auto jj = graph->edge_begin(node), eejj = graph->edge_end(node);
       jj != eejj; ++jj)
    assert(node == graph->getEdgeDst(jj) || graph->getData(graph->getEdgeDst(jj)).isMatched());
}

void assertNoMatched(GGraph* graph) {
  for (auto nn = graph->begin(), en = graph->end(); nn != en; ++nn)
    assert(!graph->getData(*nn).isMatched());
}

struct HEMmatch {
  std::pair<GNode, int> operator()(GNode node, GGraph* graph, bool tag) {
    GNode retval = node; // match self if nothing else
    int maxwgt = std::numeric_limits<int>::min();
    //    nume += std::distance(graph->edge_begin(node), graph->edge_end(node));
    for (auto jj = graph->edge_begin(node, Galois::MethodFlag::NONE), eejj = graph->edge_end(node);
         jj != eejj; ++jj) {
      //      ++checked;
      GNode neighbor = graph->getEdgeDst(jj);
      MetisNode& neighMNode = graph->getData(neighbor, Galois::MethodFlag::NONE);
      int edgeData = graph->getEdgeData(jj, Galois::MethodFlag::NONE);
      if (!neighMNode.isMatched() && neighbor != node && maxwgt < edgeData) {
        maxwgt = edgeData;
        retval = neighbor;
      }
    }
    return std::make_pair(retval, maxwgt);;
  }
  GNode operator()(GNode node, GGraph* graph) {
    return operator()(node, graph, true).first;
  }
};


struct RMmatch {
  GNode operator()(GNode node, GGraph* graph) {
    for (auto jj = graph->edge_begin(node, Galois::MethodFlag::NONE), eejj = graph->edge_end(node);
         jj != eejj; ++jj) {
      GNode neighbor = graph->getEdgeDst(jj);
      if (!graph->getData(neighbor, Galois::MethodFlag::NONE).isMatched() && neighbor != node)
        return neighbor;
    }
    return node;
    //Don't actually do random, just choose first
  }
  std::pair<GNode, int> operator()(GNode node, GGraph* graph, bool tag) {
    return std::make_pair(operator()(node, graph), 0);
  }
};

template<typename MatchingPolicy>
struct TwoHopMatcher {
  MatchingPolicy matcher;
  GNode operator()(GNode node, GGraph* graph) {
    std::pair<GNode, int> retval(node, std::numeric_limits<int>::min());
    for (auto jj = graph->edge_begin(node, Galois::MethodFlag::NONE), eejj = graph->edge_end(node);
         jj != eejj; ++jj) {
      GNode neighbor = graph->getEdgeDst(jj);
      std::pair<GNode, int> tval = matcher(neighbor, graph, true);
      if (tval.first != node && tval.first != neighbor && tval.second > retval.second)
        retval = tval;
    }
    return retval.first;
  }
};

/*
 *This operator is responsible for matching.
 1. There are two types of matching. Random and Heavy Edge matching 
 2. Random matching picks any random node above a threshold and matches the nodes. RM.h 
 3. Heavy Edge Matching matches the vertex which is connected by the heaviest edge. HEM.h 
 4. This operator can also create the multinode, i.e. the node which is created on combining two matched nodes.  
 5. You can enable/disable 4th by changing variantMetis::mergeMatching
*/

typedef Galois::InsertBag<GNode> NodeBag;
typedef Galois::GReducible<unsigned, Galois::ReduceAssignWrap<std::plus<unsigned> > > Pcounter;


template<typename MatchingPolicy>
struct parallelMatchAndCreateNodes {
  MatchingPolicy matcher;
  GGraph *fineGGraph;
  GGraph *coarseGGraph;
  Pcounter& pc;
  NodeBag& noEdgeBag;
  bool selfMatch;

  parallelMatchAndCreateNodes(MetisGraph* Graph, Pcounter& pc, NodeBag& edBag, bool selfMatch)
    : matcher(),
      fineGGraph(Graph->getFinerGraph()->getGraph()), 
      coarseGGraph(Graph->getGraph()), 
      pc(pc),
      noEdgeBag(edBag),
      selfMatch(selfMatch) {
    assert(fineGGraph != coarseGGraph);
  }

  void operator()(GNode item, Galois::UserContext<GNode> &lwl) {
    if (fineGGraph->getData(item).isMatched())
      return;
    if(fineGGraph->edge_begin(item, Galois::MethodFlag::NONE) == fineGGraph->edge_end(item, Galois::MethodFlag::NONE)){
      noEdgeBag.push(item);
      return;
    }
    GNode ret;
    do {
      ret = matcher(item, fineGGraph);
      //lock ret, since we found it lock-free it may be matched, so try again
    } while (fineGGraph->getData(ret).isMatched());

    //at this point both ret and item (and failed matches) are locked.
    //We do not leave the above loop until we both have the lock on
    //the node and check the matched status of the locked node.  the
    //lock before (final) read ensures that we will see any write to matched

    unsigned numEdges = std::distance(fineGGraph->edge_begin(item, Galois::MethodFlag::NONE), fineGGraph->edge_end(item, Galois::MethodFlag::NONE));
    //assert(numEdges == std::distance(fineGGraph->edge_begin(item), fineGGraph->edge_end(item)));

    GNode N;
    if (ret != item) {
      //match found
      numEdges += std::distance(fineGGraph->edge_begin(ret, Galois::MethodFlag::NONE), fineGGraph->edge_end(ret, Galois::MethodFlag::NONE));
      //Cautious point
      N = coarseGGraph->createNode(numEdges, 
                                   fineGGraph->getData(item).getWeight() +
                                   fineGGraph->getData(ret).getWeight(),
                                   item, ret);
      fineGGraph->getData(item).setMatched();
      fineGGraph->getData(ret).setMatched();
      fineGGraph->getData(item).setParent(N);
      fineGGraph->getData(ret).setParent(N);
    } else {
      //assertAllMatched(item, fineGGraph);
      //Cautious point
      //no match
      if (selfMatch) {
        pc.update(1);
        N = coarseGGraph->createNode(numEdges, fineGGraph->getData(item).getWeight(), item);
        fineGGraph->getData(item).setMatched();
        fineGGraph->getData(item).setParent(N);
      }
    }
  }
};

/*
 * This operator is responsible for doing a union find of the edges
 * between matched nodes and populate the edges in the coarser graph
 * node.
 */

struct parallelPopulateEdges {
  typedef int tt_does_not_need_push;
  typedef int tt_needs_per_iter_alloc;
    
  GGraph *coarseGGraph;
  GGraph *fineGGraph;
  parallelPopulateEdges(MetisGraph *Graph)
    :coarseGGraph(Graph->getGraph()), fineGGraph(Graph->getFinerGraph()->getGraph()) {
    assert(fineGGraph != coarseGGraph);
  }

  template<typename Context>
  void goSort(GNode node, Context& lwl) {
    //    std::cout << 'p';
    //fineGGraph is read only in this loop, so skip locks
    MetisNode &nodeData = coarseGGraph->getData(node, Galois::MethodFlag::NONE);

    typedef std::deque<std::pair<GNode, unsigned>, Galois::PerIterAllocTy::rebind<std::pair<GNode,unsigned> >::other> GD;
    //copy and translate all edges
    GD edges(GD::allocator_type(lwl.getPerIterAlloc()));

    for (unsigned x = 0; x < nodeData.numChildren(); ++x)
      for (auto ii = fineGGraph->edge_begin(nodeData.getChild(x), Galois::MethodFlag::NONE), ee = fineGGraph->edge_end(nodeData.getChild(x)); ii != ee; ++ii) {
        GNode dst = fineGGraph->getEdgeDst(ii);
        GNode p = fineGGraph->getData(dst, Galois::MethodFlag::NONE).getParent();
        edges.emplace_back(p, fineGGraph->getEdgeData(ii, Galois::MethodFlag::NONE));
      }
    
    //slightly faster not ordering by edge weight
    std::sort(edges.begin(), edges.end(), [] (const std::pair<GNode, unsigned>& lhs, const std::pair<GNode, unsigned>& rhs) { return lhs.first < rhs.first; } );

    //insert edges
    for (auto pp = edges.begin(), ep = edges.end(); pp != ep;) {
      GNode dst = pp->first;
      unsigned sum = pp->second;
      ++pp;
      if (node != dst) { //no self edges
        while (pp != ep && pp->first == dst) {
          sum += pp->second;
          ++pp;
        }
        coarseGGraph->addEdgeWithoutCheck(node, dst, Galois::MethodFlag::NONE, sum);
      }
    }
    //    assert(e);
    //nodeData.setNumEdges(e);
  }

  template<typename Context>
  void operator()(GNode node, Context& lwl) {
    // MetisNode &nodeData = coarseGGraph->getData(node, Galois::MethodFlag::NONE);
    // if (std::distance(fineGGraph->edge_begin(nodeData.getChild(0), Galois::MethodFlag::NONE),
    //                   fineGGraph->edge_begin(nodeData.getChild(0), Galois::MethodFlag::NONE))
    //     < 256)
    //   goSort(node,lwl);
    // else
    //   goHM(node,lwl);
    goSort(node, lwl);
    //goHeap(node,lwl);
  }
};

struct HighDegreeIndexer: public std::unary_function<GNode, unsigned int> {
  static GGraph* indexgraph;
  unsigned int operator()(const GNode& val) const {
    return indexgraph->getData(val, Galois::MethodFlag::NONE).isFailedMatch() ?
      std::numeric_limits<unsigned int>::max() :
      (std::numeric_limits<unsigned int>::max() - 
       ((std::distance(indexgraph->edge_begin(val, Galois::MethodFlag::NONE), 
                       indexgraph->edge_end(val, Galois::MethodFlag::NONE))) >> 2));
  }
};

GGraph* HighDegreeIndexer::indexgraph = 0;

struct LowDegreeIndexer: public std::unary_function<GNode, unsigned int> {
  unsigned int operator()(const GNode& val) const {
    unsigned x =std::distance(HighDegreeIndexer::indexgraph->edge_begin(val, Galois::MethodFlag::NONE), 
                              HighDegreeIndexer::indexgraph->edge_end(val, Galois::MethodFlag::NONE));
    return x; // >> 2;
    // int targetlevel = 0;
    // while (x >>= 1) ++targetlevel;
    // return targetlevel;

  }
};

struct WeightIndexer: public std::unary_function<GNode, int> {
  int operator()(const GNode& val) const {
    return HighDegreeIndexer::indexgraph->getData(val, Galois::MethodFlag::NONE).getWeight();
  }
};

unsigned minRuns(unsigned coarsenTo, unsigned size) {
  unsigned num = 0;
  while (coarsenTo < size) {
    ++num;
    size /= 2;
  }
  return num;
}

unsigned fixupLoners(NodeBag& b, GGraph* coarseGGraph, GGraph* fineGGraph) {
  unsigned count = 0;
  auto ii = b.begin(), ee = b.end();
  while (ii != ee) {
    auto i2 = ii;
    ++i2;
    if (i2 != ee) {
      GNode N = coarseGGraph->createNode(0, 
                                   fineGGraph->getData(*ii).getWeight() +
                                   fineGGraph->getData(*i2).getWeight(),
                                   *ii, *i2);
      fineGGraph->getData(*ii).setMatched();
      fineGGraph->getData(*i2).setMatched();
      fineGGraph->getData(*ii).setParent(N);
      fineGGraph->getData(*i2).setParent(N);
      ++ii;
      ++count;
    } else {
      GNode N = coarseGGraph->createNode(0, 
                                         fineGGraph->getData(*ii).getWeight(),
                                         *ii);
      fineGGraph->getData(*ii).setMatched();
      fineGGraph->getData(*ii).setParent(N);
    }
    ++ii;
  }
  return count;
}

unsigned findMatching(MetisGraph* coarseMetisGraph, bool useRM, bool use2Hop, bool verbose) {
  MetisGraph* fineMetisGraph = coarseMetisGraph->getFinerGraph();

  /*
   * Different worklist versions tried, dChunkedFIFO 256 works best with LC_MORPH_graph.
   * Another good type would be Lazy Iter.
   */
  //typedef Galois::WorkList::ChunkedLIFO<64, GNode> WL;
  //typedef Galois::WorkList::LazyIter<decltype(fineGGraph->local_begin()),false> WL;

  NodeBag bagOfLoners;
  Pcounter pc;

  bool useOBIM = true;

  typedef decltype(fineMetisGraph->getGraph()->local_begin()) ITY;
  typedef Galois::WorkList::StableIterator<ITY, true> WL;
  //typedef Galois::WorkList::Random<> WL;
  if(useRM) {
    parallelMatchAndCreateNodes<RMmatch> pRM(coarseMetisGraph, pc, bagOfLoners, !use2Hop);
    Galois::for_each_local(*fineMetisGraph->getGraph(), pRM, Galois::loopname("match"), Galois::wl<WL>());
  } else {
    //FIXME: use obim for SHEM matching
    typedef Galois::WorkList::dChunkedLIFO<16> Chunk;
    typedef Galois::WorkList::OrderedByIntegerMetric<WeightIndexer, Chunk> pW;
    typedef Galois::WorkList::OrderedByIntegerMetric<LowDegreeIndexer, Chunk> pLD;
    typedef Galois::WorkList::OrderedByIntegerMetric<HighDegreeIndexer, Chunk> pHD;

    HighDegreeIndexer::indexgraph = fineMetisGraph->getGraph();
    parallelMatchAndCreateNodes<HEMmatch> pHEM(coarseMetisGraph, pc, bagOfLoners, !use2Hop);
    if (useOBIM)
      Galois::for_each_local(*fineMetisGraph->getGraph(), pHEM, Galois::loopname("match"), Galois::wl<pLD>());
    else
      Galois::for_each_local(*fineMetisGraph->getGraph(), pHEM, Galois::loopname("match"), Galois::wl<WL>());
  }
  unsigned c = fixupLoners(bagOfLoners, coarseMetisGraph->getGraph(), fineMetisGraph->getGraph());
  if (verbose && c)
    std::cout << "\n\tLone Matches " << c;
  if (use2Hop) {
    typedef Galois::WorkList::dChunkedLIFO<16> Chunk;
    typedef Galois::WorkList::OrderedByIntegerMetric<WeightIndexer, Chunk> pW;
    typedef Galois::WorkList::OrderedByIntegerMetric<LowDegreeIndexer, Chunk> pLD;
    typedef Galois::WorkList::OrderedByIntegerMetric<HighDegreeIndexer, Chunk> pHD;

    HighDegreeIndexer::indexgraph = fineMetisGraph->getGraph();
    Pcounter pc2;
    parallelMatchAndCreateNodes<TwoHopMatcher<HEMmatch> > p2HEM(coarseMetisGraph, pc2, bagOfLoners, true);
    if (useOBIM)
      Galois::for_each_local(*fineMetisGraph->getGraph(), p2HEM, Galois::loopname("match"), Galois::wl<pLD>());
    else
      Galois::for_each_local(*fineMetisGraph->getGraph(), p2HEM, Galois::loopname("match"), Galois::wl<WL>());
    return pc2.reduce();
  }
  return pc.reduce();
}

void createCoarseEdges(MetisGraph *coarseMetisGraph) {
  MetisGraph* fineMetisGraph = coarseMetisGraph->getFinerGraph();
  GGraph* fineGGraph = fineMetisGraph->getGraph();
  typedef Galois::WorkList::StableIterator<decltype(fineGGraph->local_begin()), true> WL;
  parallelPopulateEdges pPE(coarseMetisGraph);
  Galois::for_each_local(*coarseMetisGraph->getGraph(), pPE, Galois::loopname("popedge"), Galois::wl<WL>());
}

MetisGraph* coarsenOnce(MetisGraph *fineMetisGraph, unsigned& rem, bool useRM, bool with2Hop, bool verbose) {
  MetisGraph *coarseMetisGraph = new MetisGraph(fineMetisGraph);
  Galois::Timer t, t2;
  if (verbose)
    t.start();
  rem = findMatching(coarseMetisGraph, useRM, with2Hop, verbose);
  if (verbose) {
    t.stop();
    std::cout << "\n\tTime Matching " << t.get() << "\n";
    t2.start();
  }
  createCoarseEdges(coarseMetisGraph);
  if (verbose) {
    t2.stop();
    std::cout << "\tTime Creating " << t2.get() << "\n";
  }
  return coarseMetisGraph;
}

} // anon namespace

MetisGraph* coarsen(MetisGraph* fineMetisGraph, unsigned coarsenTo, bool verbose) {
  MetisGraph* coarseGraph = fineMetisGraph;
  unsigned size = std::distance(fineMetisGraph->getGraph()->begin(), fineMetisGraph->getGraph()->end());
  unsigned iterNum = 0;
  bool with2Hop = false;
  unsigned stat;
  while (true) {//overflow
    if (verbose) {
      std::cout << "Coarsening " << iterNum << "\t";
      stat = graphStat(coarseGraph->getGraph());
    }
    unsigned rem = 0;
    coarseGraph = coarsenOnce(coarseGraph, rem, false, with2Hop, verbose);
    unsigned newSize = size / 2 + rem / 2;
    if (verbose) {
      std::cout << "\tTO\t";
      unsigned stat2 = graphStat(coarseGraph->getGraph());
      std::cout << "\n\tRatio " << (double)stat2 / (double)stat << " REM " << rem << " new size " << newSize << "\n";
    }

    if (size * 3 < newSize * 4) {
      with2Hop = true;
      if (verbose)
        std::cout << "** Enabling 2 hop matching\n";
    } else {
      with2Hop = false;
    }

    size = newSize;
    if (newSize * 4 < coarsenTo) { //be more exact near the end
      size = std::distance(coarseGraph->getGraph()->begin(), coarseGraph->getGraph()->end());
      if (size < coarsenTo)
        break;
    }
    ++iterNum;
  }
  
  return coarseGraph;
}
