/** AbstractMain holds common functionality for main classes -*- C++ -*-
 * @file
 * @section License
 *
 * Galois, a framework to exploit amorphous data-parallelism in irregular
 * programs.
 *
 * Copyright (C) 2011, The University of Texas at Austin. All rights reserved.
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
 * Created on: Jun 24, 2011
 *
 * @author M. Amber Hassaan <ahassaan@ices.utexas.edu>
 */

#ifndef DES_ABSTRACT_MAIN_H_
#define DES_ABSTRACT_MAIN_H_

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <utility>

#include <cstdio>

#include "Galois/Statistic.h"
#include "Galois/Graph/Graph.h"
#include "Galois/Graph/LCGraph.h"
#include "Galois/Galois.h"
#include "Galois/Runtime/Sampling.h"

#include "llvm/Support/CommandLine.h"

#include "Lonestar/BoilerPlate.h"

#include "comDefs.h"
#include "BaseSimObject.h"
#include "Event.h"
#include "SimInit.h"
#include "SimGate.h"
#include "Input.h"
#include "Output.h"

namespace des {

namespace cll = llvm::cl;

static const char* name = "Discrete Event Simulation";
static const char* desc = "Perform logic circuit simulation using Discrete Event Simulation";
static const char* url = "discrete_event_simulation";

static cll::opt<std::string> netlistFile(cll::Positional, cll::desc("<input file>"), cll::Required);

/**
 * The Class AbstractMain holds common functionality for {@link des_unord::DESunorderedSerial} and {@link des_unord::DESunordered}.
 */
// TODO: graph type can also be exposed to sub-classes as a template parameter
template <typename SimInit_tp>
class AbstractMain {

public:

  typedef Galois::Graph::FirstGraph<typename SimInit_tp::BaseSimObj_ty*, void, true> Graph;
  typedef typename Graph::GraphNode GNode;


protected:
  static const unsigned CHUNK_SIZE = 8;

  static const unsigned DEFAULT_EPI = 32;

  /**
   * Gets the version.
   *
   * @return the version
   */
  virtual std::string getVersion() const = 0;

  /**
   * Run loop.
   *
   * @throws Exception the exception
   */
  virtual void runLoop(const SimInit_tp& simInit, Graph& graph) = 0;


  virtual void initRemaining (const SimInit_tp& simInit, Graph& graph) = 0;

public:
  /**
   * Run the simulation
   * @param argc
   * @param argv
   */
  void run(int argc, char* argv[]) {

    Galois::StatManager sm;
    LonestarStart(argc, argv, name, desc, url);

    SimInit_tp simInit(netlistFile);
    Graph graph;
    simInit.initialize (graph);

    // Graph graph;
    // graph.copyFromGraph (in_graph);

    printf("circuit graph: %d nodes, %zd edges\n", graph.size(), simInit.getNumEdges());
    printf("Number of initial events = %zd\n", simInit.getInitEvents().size());

    initRemaining (simInit, graph);

    Galois::StatTimer t;

    t.start ();
    Galois::Runtime::beginSampling ();

    runLoop(simInit, graph);

    Galois::Runtime::endSampling ();
    t.stop ();

    if (!skipVerify) {
      simInit.verify ();
    }

  }

};

} // namespace des
#endif // DES_ABSTRACT_MAIN_H_ 
