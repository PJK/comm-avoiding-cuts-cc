/** Connected components -*- C++ -*-
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
 * @section Description
 *
 * Compute the connect components of a graph and optionally write out the largest
 * component to file.
 *
 * @author Donald Nguyen <ddn@cs.utexas.edu>
 */
#include "Galois/Galois.h"
#include "Galois/Accumulator.h"
#include "Galois/Bag.h"
#include "Galois/DomainSpecificExecutors.h"
#include "Galois/Statistic.h"
#include "Galois/UnionFind.h"
#include "Galois/Graph/LCGraph.h"
#include "Galois/Graph/OCGraph.h"
#include "Galois/Graph/TypeTraits.h"
#include "Galois/ParallelSTL/ParallelSTL.h"
#include "llvm/Support/CommandLine.h"

#include "utils.hpp"

#include <utility>
#include <vector>
#include <algorithm>
#include <iostream>

#ifdef GALOIS_USE_EXP
#include "LigraAlgo.h"
#include "GraphLabAlgo.h"
#include "GraphChiAlgo.h"
#endif

const char* name = "Connected Components";
const char* desc = "Computes the connected components of a graph";
const char* url = 0;

enum Algo {
  async,
  asyncOc,
  blockedasync,
  graphchi,
  graphlab,
  labelProp,
  ligra,
  ligraChi,
  serial,
  synchronous
};

enum WriteType {
  none,
  largest
};

namespace cll = llvm::cl;
static cll::opt<std::string> inputFilename(cll::Positional, cll::desc("<input file>"), cll::Required);
static cll::opt<std::string> outputFilename(cll::Positional, cll::desc("[output file]"), cll::init("largest.gr"));
static cll::opt<std::string> transposeGraphName("graphTranspose", cll::desc("Transpose of input graph"));
static cll::opt<bool> symmetricGraph("symmetricGraph", cll::desc("Input graph is symmetric"), cll::init(false));
cll::opt<unsigned int> memoryLimit("memoryLimit",
	cll::desc("Memory limit for out-of-core algorithms (in MB)"), cll::init(~0U));
static cll::opt<WriteType> writeType("output", cll::desc("Output type:"),
	cll::values(
	  clEnumValN(WriteType::none, "none", "None (default)"),
	  clEnumValN(WriteType::largest, "largest", "Write largest component"),
	  clEnumValEnd), cll::init(WriteType::none));
static cll::opt<Algo> algo("algo", cll::desc("Choose an algorithm:"),
	cll::values(
	  clEnumValN(Algo::async, "async", "Asynchronous (default)"),
#ifdef GALOIS_USE_EXP
	  clEnumValN(Algo::graphchi, "graphchi", "Using GraphChi programming model"),
	  clEnumValN(Algo::graphlab, "graphlab", "Using GraphLab programming model"),
	  clEnumValN(Algo::ligraChi, "ligraChi", "Using Ligra and GraphChi programming model"),
	  clEnumValN(Algo::ligra, "ligra", "Using Ligra programming model"),
#endif
	  clEnumValEnd), cll::init(Algo::async));

struct Node: public Galois::UnionFindNode<Node> {
  typedef Node* component_type;
  unsigned int id;

  component_type component() { return this->findAndCompress(); }
};

template<typename Graph>
void readInOutGraph(Graph& graph) {
  using namespace Galois::Graph;
  if (symmetricGraph) {
	Galois::Graph::readGraph(graph, inputFilename);
  } else if (transposeGraphName.size()) {
	Galois::Graph::readGraph(graph, inputFilename, transposeGraphName);
  } else {
	GALOIS_DIE("Graph type not supported");
  }
}
/**
 * Like synchronous algorithm, but if we restrict path compression (as done is
 * @link{UnionFindNode}), we can perform unions and finds concurrently.
 */
struct AsyncAlgo {
  typedef Galois::Graph::LC_CSR_Graph<Node,void>
	::with_numa_alloc<true>::type
	::with_no_lockable<true>::type
	Graph;
  typedef Graph::GraphNode GNode;

  void readGraph(Graph& graph) { Galois::Graph::readGraph(graph, inputFilename); }

  struct Merge {
	typedef int tt_does_not_need_aborts;
	typedef int tt_does_not_need_push;

	Graph& graph;
	Galois::Statistic& emptyMerges;
	Merge(Graph& g, Galois::Statistic& e): graph(g), emptyMerges(e) { }

	//! Add the next edge between components to the worklist
	void operator()(const GNode& src, Galois::UserContext<GNode>&) const {
	  (*this)(src);
	}

	void operator()(const GNode& src) const {
	  Node& sdata = graph.getData(src, Galois::MethodFlag::NONE);

	  for (Graph::edge_iterator ii = graph.edge_begin(src, Galois::MethodFlag::NONE),
		  ei = graph.edge_end(src, Galois::MethodFlag::NONE); ii != ei; ++ii) {
		GNode dst = graph.getEdgeDst(ii);
		Node& ddata = graph.getData(dst, Galois::MethodFlag::NONE);

		if (symmetricGraph && src >= dst)
		  continue;

		if (!sdata.merge(&ddata))
		  emptyMerges += 1;
	  }
	}
  };

  void operator()(Graph& graph) {
	Galois::Statistic emptyMerges("EmptyMerges");
	Galois::for_each_local(graph, Merge(graph, emptyMerges));
  }
};



typedef typename AsyncAlgo::Graph Graph;

int main(int argc, char* argv[]) {
  int iterations = 1;

	int numThreads = std::stoi(argv[2]);
	Galois::setActiveThreads(numThreads);

  if (argc == 4) {
	iterations = std::stoi(argv[3]);
  }

  AsyncAlgo algo;
  Graph graph;
  Galois::Graph::readGraph(graph, argv[1]);

  for (int i = 0; i < iterations; i++) {
	double time;

	unsigned int id = 0;
	for (typename Graph::iterator ii = graph.begin(), ei = graph.end(); ii != ei; ++ii, ++id) {
	  graph.getData(*ii).id = id;
	}


	PAPI_START();
	TimeUtils::measure<void>([&]() {
		Galois::preAlloc(numThreads + (2 * graph.size() * sizeof(typename Graph::node_data_type)) / Galois::Runtime::MM::pageSize);
		algo(graph);
	}, time);
	PAPI_STOP(0, 0);

	std::cout << std::fixed;
	std::cout << argv[1] << ","
			  << ","
			  << numThreads << ","
			  << graph.size() << ","
			  << graph.sizeEdges() << ","
			  << time <<  ","
			  << ","
			  << "galois_cc" << ","
			  << std::endl;
  }
}

