/** Tutorial torus application -*- C++ -*-
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
 * Simple tutorial application. Creates a torus graph and each node increments
 * its neighbors data by one. 
 *
 * @author Donald Nguyen <ddn@cs.utexas.edu>
 */
#include "Galois/Galois.h"
#include "Galois/Statistic.h"
#include "Galois/Graph/Graph.h"
#include <iostream>

//! Graph has int node data, void edge data and is directed
typedef Galois::Graph::FirstGraph<int,void,true> Graph;
//! Opaque pointer to graph node
typedef Graph::GraphNode GNode;

//! Increments node value of each neighbor by 1
struct IncrementNeighbors {
  Graph& g;
  IncrementNeighbors(Graph& g): g(g) { }

  //! Operator. Context parameter is unused in this example.
  void operator()(GNode n, Galois::UserContext<GNode>& ctx) {
    // For each outgoing edge (n, dst)
    for (Graph::edge_iterator ii = g.edge_begin(n), ei = g.edge_end(n); ii != ei; ++ii) {
      GNode dst = g.getEdgeDst(ii);
      int& data = g.getData(dst);
      // Increment node data by 1
      data += 1;
    }
  }
};

//! Returns true if node value equals v
struct ValueEqual {
  Graph& g;
  int v;
  ValueEqual(Graph& g, int v): g(g), v(v) { }
  bool operator()(GNode n) {
    return g.getData(n) == v;
  }
};

//! Construct a simple torus graph
void constructTorus(Graph& g, int height, int width) {
  // Construct set of nodes
  int numNodes = height * width;
  std::vector<GNode> nodes(numNodes);
  for (int i = 0; i < numNodes; ++i) {
    GNode n = g.createNode(0);
    g.addNode(n);
    nodes[i] = n;
  }

  // Add edges
  for (int x = 0; x < width; ++x) {
    for (int y = 0; y < height; ++y) {
      GNode c = nodes[x*height + y];
      GNode n = nodes[x*height + ((y+1) % height)];
      GNode s = nodes[x*height + ((y-1+height) % height)];
      GNode e = nodes[((x+1) % width)*height + y];
      GNode w = nodes[((x-1+width) % width)*height + y];
      g.addEdge(c, n);
      g.addEdge(c, s);
      g.addEdge(c, e);
      g.addEdge(c, w);
    }
  }
}

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "<num threads> <sqrt grid size>\n";
    return 1;
  }
  unsigned int numThreads = atoi(argv[1]);
  int n = atoi(argv[2]);

  numThreads = Galois::setActiveThreads(numThreads);
  std::cout << "Using " << numThreads << " thread(s) and " << n << " x " << n << " torus\n";

  Graph graph;
  constructTorus(graph, n, n);

  Galois::StatTimer T;
  T.start();
  Galois::for_each(graph.begin(), graph.end(), IncrementNeighbors(graph));
  T.stop();

  std::cout << "Elapsed time: " << T.get() << " milliseconds\n";

  // Verify
  int count = std::count_if(graph.begin(), graph.end(), ValueEqual(graph, 4));
  if (count != n * n) {
    std::cerr << "Expected " << n * n << " nodes with value = 4 but found " << count << " instead.\n";
    return 1;
  } else {
    std::cout << "Correct!\n";
  }

  return 0;
}
