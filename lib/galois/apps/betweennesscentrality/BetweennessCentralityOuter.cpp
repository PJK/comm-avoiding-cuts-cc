/** Betweenness centrality application -*- C++ -*-
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
 * @author Dimitrios Prountzos <dprountz@cs.utexas.edu>
 */
#include "Galois/Galois.h"
#include "Galois/Accumulator.h"
#include "Galois/Statistic.h"
#include "Galois/UserContext.h"
#include "Galois/Graph/LCGraph.h"

#include "llvm/Support/CommandLine.h"
#include "Lonestar/BoilerPlate.h"

#include <boost/iterator/filter_iterator.hpp>

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <vector>
#include <deque>
#include <cstdlib>

static const char* name = "Betweenness Centrality";
static const char* desc = "Computes the betweenness centrality of all nodes in a graph";
static const char* url  = "betweenness_centrality";

static llvm::cl::opt<std::string> filename(llvm::cl::Positional, llvm::cl::desc("<input file>"), llvm::cl::Required);
static llvm::cl::opt<int> iterLimit("limit", llvm::cl::desc("Limit number of iterations to value (0 is all nodes)"), llvm::cl::init(0));
static llvm::cl::opt<unsigned int> startNode("startNode", llvm::cl::desc("Node to start search from"), llvm::cl::init(0));
static llvm::cl::opt<bool> forceVerify("forceVerify", llvm::cl::desc("Abort if not verified, only makes sense for torus graphs"));
static llvm::cl::opt<bool> printAll("printAll", llvm::cl::desc("Print betweenness values for all nodes"));

typedef Galois::Graph::LC_CSR_Graph<void, void>::with_no_lockable<true>::type
  ::with_numa_alloc<true>::type Graph;
typedef Graph::GraphNode GNode;

Graph* G;
int NumNodes;

Galois::Runtime::PerThreadStorage<double*> CB;
Galois::Runtime::PerThreadStorage<double*> perThreadSigma;
Galois::Runtime::PerThreadStorage<int*> perThreadD;
Galois::Runtime::PerThreadStorage<double*> perThreadDelta;
Galois::Runtime::PerThreadStorage<Galois::gdeque<GNode>*> perThreadSucc;

template<typename T>
struct PerIt {  
  typedef typename Galois::PerIterAllocTy::rebind<T>::other Ty;
};

struct process {
  typedef int tt_does_not_need_aborts;
  //typedef int tt_needs_per_iter_alloc;
  typedef int tt_does_not_need_push;

  void operator()(GNode& _req, Galois::UserContext<GNode>& lwl) {
    Galois::gdeque<GNode> SQ;
    double* sigma = *perThreadSigma.getLocal();
    int* d = *perThreadD.getLocal();
    double* delta = *perThreadDelta.getLocal();
    Galois::gdeque<GNode>* succ = *perThreadSucc.getLocal();

#if 0
    std::deque<double, PerIt<double>::Ty> sigma(NumNodes, 0.0, lwl.getPerIterAlloc());
    std::deque<int, PerIt<int>::Ty> d(NumNodes, 0, lwl.getPerIterAlloc());
    std::deque<double, PerIt<double>::Ty> delta(NumNodes, 0.0, lwl.getPerIterAlloc());
    std::deque<GNdeque, PerIt<GNdeque>::Ty> succ(NumNodes, GNdeque(lwl.getPerIterAlloc()), lwl.getPerIterAlloc());
#endif    
    unsigned int QAt = 0;
    
    int req = _req;
    
    sigma[req] = 1;
    d[req] = 1;
    
    SQ.push_back(_req);
    for (auto qq = SQ.begin(), eq = SQ.end(); qq != eq; ++qq) {
      GNode _v = *qq;
      int v = _v;
      for (Graph::edge_iterator
          ii = G->edge_begin(_v, Galois::MethodFlag::NONE),
          ee = G->edge_end(_v, Galois::MethodFlag::NONE); ii != ee; ++ii) {
	GNode _w = G->getEdgeDst(ii);
	int w = _w;
	if (!d[w]) {
	  SQ.push_back(_w);
	  d[w] = d[v] + 1;
	}
	if (d[w] == d[v] + 1) {
	  sigma[w] = sigma[w] + sigma[v];
	  succ[v].push_back(w);
	}
      }
    }
    while (SQ.size() > 1) {
      int w = SQ.back();
      SQ.pop_back();

      double sigma_w = sigma[w];
      double delta_w = delta[w];
      auto& slist = succ[w];
      for (auto ii = slist.begin(), ee = slist.end(); ii != ee; ++ii) {
	//std::cerr << "Processing node " << w << std::endl;
	GNode v = *ii;
	delta_w += (sigma_w/sigma[v])*(1.0 + delta[v]);
      }
      delta[w] = delta_w;
    }
    double* Vec = *CB.getLocal();
    for (unsigned int i = 0; i < NumNodes; ++i) {
      Vec[i] += delta[i];
      delta[i] = 0;
      sigma[i] = 0;
      d[i] = 0;
      succ[i].clear();
    }
  }
};

// Verification for reference torus graph inputs. 
// All nodes should have the same betweenness value.
void verify() {
  double sampleBC = 0.0;
  bool firstTime = true;
  for (int i=0; i<NumNodes; ++i) {
    double bc = (*CB.getRemote(0))[i];
    for (unsigned int j = 1; j < Galois::getActiveThreads(); ++j)
      bc += (*CB.getRemote(j))[i];
    if (firstTime) {
      sampleBC = bc;
      std::cerr << "BC: " << sampleBC << std::endl;
      firstTime = false;
    } else {
      if (!((bc - sampleBC) <= 0.0001)) {
        std::cerr << "If torus graph, verification failed " << (bc - sampleBC) << "\n";
        if (forceVerify)
          abort();
        assert ((bc - sampleBC) <= 0.0001);
        return;
      }
    }
  }
}

void printBCcertificate() {
  std::stringstream foutname;
  foutname << "outer_certificate_" << numThreads;
  std::ofstream outf(foutname.str().c_str());
  std::cerr << "Writing certificate..." << std::endl;

  for (int i=0; i<NumNodes; ++i) {
    double bc = (*CB.getRemote(0))[i];
    for (unsigned int j = 1; j < Galois::getActiveThreads(); ++j)
      bc += (*CB.getRemote(j))[i];
    outf << i << ": " << std::setiosflags(std::ios::fixed) << std::setprecision(9) << bc << std::endl;
  }
  outf.close();
}

struct HasOut: public std::unary_function<GNode,bool> {
  Graph* graph;
  HasOut(Graph* g): graph(g) { }
  bool operator()(const GNode& n) const {
    return graph->edge_begin(n) != graph->edge_end(n);
  }
};

struct InitializeLocal {
  template<typename T>
  void initArray(T** addr) {
    T* a = new T[NumNodes]();
    *addr = a;
  }
  void operator()(unsigned, unsigned) {
    initArray(CB.getLocal());
    initArray(perThreadSigma.getLocal());
    initArray(perThreadD.getLocal());
    initArray(perThreadDelta.getLocal());
    initArray(perThreadSucc.getLocal());
  }
};

struct DeleteLocal {
  template<typename T>
  void deleteArray(T** addr) {
    delete [] *addr;
  }
  void operator()(unsigned, unsigned) {
    deleteArray(CB.getLocal());
    deleteArray(perThreadSigma.getLocal());
    deleteArray(perThreadD.getLocal());
    deleteArray(perThreadDelta.getLocal());
    deleteArray(perThreadSucc.getLocal());
  }
};

int main(int argc, char** argv) {
  Galois::StatManager M;
  LonestarStart(argc, argv, name, desc, url);

  Graph g;
  G = &g;
  Galois::Graph::readGraph(*G, filename);

  NumNodes = G->size();

  Galois::on_each(InitializeLocal());

  Galois::reportPageAlloc("MeminfoPre");
  Galois::preAlloc(numThreads * NumNodes / 1650);
  Galois::reportPageAlloc("MeminfoMid");

  boost::filter_iterator<HasOut,Graph::iterator>
    begin  = boost::make_filter_iterator(HasOut(G), g.begin(), g.end()),
    end    = boost::make_filter_iterator(HasOut(G), g.end(), g.end());

  boost::filter_iterator<HasOut,Graph::iterator> begin2 = 
    iterLimit ? Galois::safe_advance(begin, end, (int)iterLimit) : end;

  size_t iterations = std::distance(begin, begin2);

  std::vector<GNode> v(begin, begin2);

  std::cout 
    << "NumNodes: " << NumNodes
    << " Start Node: " << startNode 
    << " Iterations: " << iterations << "\n";
  
  typedef Galois::WorkList::StableIterator< std::vector<GNode>::iterator, true> WLL;
  Galois::StatTimer T;
  T.start();
  Galois::for_each(v.begin(), v.end(), process(), Galois::wl<WLL>());
  T.stop();

  if (!skipVerify) {
    for (int i=0; i<10; ++i) {
      double bc = (*CB.getRemote(0))[i];
      for (unsigned int j = 1; j < Galois::getActiveThreads(); ++j)
	bc += (*CB.getRemote(j))[i];
      std::cout << i << ": " 
        << std::setiosflags(std::ios::fixed) << std::setprecision(6) << bc << "\n";
    }
  }
  if (printAll)
    printBCcertificate();

  Galois::reportPageAlloc("MeminfoPost");

  if (forceVerify || !skipVerify) {
    verify();
  }

  // XXX(ddn): Could use unique_ptr but not supported on all our platforms :(
  Galois::on_each(DeleteLocal());

  return 0;
}
