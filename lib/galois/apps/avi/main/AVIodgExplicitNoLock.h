/** AVI unordered algorithm with no abstract locks -*- C++ -*-
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
 * @author M. Amber Hassaan <ahassaan@ices.utexas.edu>
 */

#ifndef AVI_UNORDERED_NO_LOCK_H_
#define AVI_UNORDERED_NO_LOCK_H_


#include "Galois/Graph/Graph.h"
#include "Galois/Graph/FileGraph.h"

#include "Galois/Galois.h"
#include "Galois/Atomic.h"

#include "Galois/Runtime/PerThreadStorage.h"

#include <string>
#include <sstream>
#include <limits>
#include <iostream>
#include <fstream>
#include <set>

#include <cassert>

#include "AuxDefs.h"
#include "AVI.h"
#include "Element.h"

#include "AVIabstractMain.h"
#include "AVIodgExplicit.h"

/**
 * AVI unordered algorithm that uses atomic integers 
 * and no abstract locks
 */
class AVIodgExplicitNoLock: public AVIodgExplicit {
  typedef Galois::GAtomicPadded<int> AtomicInteger;

protected:

  virtual const std::string getVersion () const {
    return "ODG explicit, no abstract locks";
  }
  
  /**
   * Functor for loop body
   */
  struct Process {

    Graph& graph;
    std::vector<AtomicInteger>& inDegVec;
    MeshInit& meshInit;
    GlobalVec& g;
    Galois::Runtime::PerThreadStorage<LocalVec>& perIterLocalVec;
    bool createSyncFiles;
    IterCounter& iter;

    Process (
        Graph& graph,
        std::vector<AtomicInteger>& inDegVec,
        MeshInit& meshInit,
        GlobalVec& g,
        Galois::Runtime::PerThreadStorage<LocalVec>& perIterLocalVec,
        bool createSyncFiles,
        IterCounter& iter):

        graph (graph),
        inDegVec (inDegVec),
        meshInit (meshInit),
        g (g),
        perIterLocalVec (perIterLocalVec),
        createSyncFiles (createSyncFiles),
        iter (iter) {}
    
    /**
     * Loop body
     *
     * The key condition is that a node and its neighbor cannot be active at the same time,
     * and it must be impossible for two of them to be processed in parallel
     * Therefore a node can add its newly active neighbors to the workset as the last step only when
     * it has finished performing all other updates. *(As per current semantics of for_each, adds to worklist
     * happen on commit. If this is not the case, then each thread should
     * accumulate adds in a temp vec and add to the worklist all together in the
     * end)
     * For the same reason, active node src must update its own in degree before updating the
     * indegree of any of the neighbors. Imagine the alternative, where active node updates its in
     * degree and that of it's neighbor in the same loop. For example A is current active node and
     * has a neighbor B. A > B, therefore A increments its own in degree and decrements that of B to
     * 1. Another active node C is neighbor of B but not of A, and C decreases in degree of B to 0
     * and adds B to the workset while A is not finished yet. This violates our key condition
     * mentioned above
     *
     * @param gn is active elemtn
     * @param lwl is the worklist handle
     * @param avi is the avi object
     */
    template <typename C>
    GALOIS_ATTRIBUTE_PROF_NOINLINE void addToWL (C& lwl, const GNode& gn, AVI* avi) {
      assert (graph.getData (gn, Galois::MethodFlag::NONE) == avi);

      if (avi->getNextTimeStamp () < meshInit.getSimEndTime ()) {
        lwl.push (gn);
      }
    }

    template <typename C>
    GALOIS_ATTRIBUTE_PROF_NOINLINE void updateODG (const GNode& src, AVI* srcAVI, C& lwl) {
      unsigned addAmt = 0;

      for (Graph::edge_iterator e = graph.edge_begin (src, Galois::MethodFlag::NONE)
          , ende = graph.edge_end (src, Galois::MethodFlag::NONE); e != ende; ++e) {

        GNode dst = graph.getEdgeDst (e);
        AVI* dstAVI = graph.getData (dst, Galois::MethodFlag::NONE);

        if (AVIComparator::compare (srcAVI, dstAVI) > 0) {
          ++addAmt;
        }

      }


      // may be the active node is still at the local minimum
      // and no updates to neighbors are necessary
      if (addAmt == 0) {
        addToWL (lwl, src, srcAVI);
      }
      else {
        inDegVec[srcAVI->getGlobalIndex ()] += addAmt;

        for (Graph::edge_iterator e = graph.edge_begin (src, Galois::MethodFlag::NONE)
            , ende = graph.edge_end (src, Galois::MethodFlag::NONE); e != ende; ++e) {

          GNode dst = graph.getEdgeDst (e);
          AVI* dstAVI = graph.getData (dst, Galois::MethodFlag::NONE);

          if (AVIComparator::compare (srcAVI, dstAVI) > 0) {
            int din = --inDegVec[dstAVI->getGlobalIndex ()];

            assert (din >= 0);

            if (din == 0) {
              addToWL (lwl, dst, dstAVI);
              // // TODO: DEBUG
              // std::cout << "Adding: " << dstAVI->toString () << std::endl;
            }
          }

        } // end for

      } // end else
    }

    template <typename ContextTy> 
    void operator () (const GNode& src, ContextTy& lwl) {
      AVI* srcAVI = graph.getData (src, Galois::MethodFlag::NONE);

      int inDeg = (int)inDegVec[srcAVI->getGlobalIndex ()];
      // assert  inDeg == 0 : String.format ("active node %s with inDeg = %d\n", srcAVI, inDeg);

//        // TODO: DEBUG
//        std::cout << "Processing element: " << srcAVI->toString() << std::endl;

      assert (inDeg == 0);

      LocalVec& l = *perIterLocalVec.getLocal();

      AVIabstractMain::simulate(srcAVI, meshInit, g, l, createSyncFiles);


      // update the inEdges count and determine
      // which neighbor is at local minimum and needs to be added to the worklist
      updateODG (src, srcAVI, lwl);


      // for debugging, remove later
      iter += 1;


    }
  };
  
public:

  /**
   * For the in degree vector, we use a vector of atomic integers
   * This along with other changes in the loop body allow us to 
   * no use abstract locks. @see Process
   */
  virtual  void runLoop (MeshInit& meshInit, GlobalVec& g, bool createSyncFiles) {
    /////////////////////////////////////////////////////////////////
    // populate an initial  worklist
    /////////////////////////////////////////////////////////////////
    std::vector<AtomicInteger> inDegVec(meshInit.getNumElements (), AtomicInteger (0));
    std::vector<GNode> initWL;

    initWorkList (initWL, inDegVec);

//    // TODO: DEBUG
//    std::cout << "Initial Worklist = " << std::endl;
//    for (size_t i = 0; i < initWL.size (); ++i) {
//      std::cout << graph.getData (initWL[i], Galois::MethodFlag::NONE)->toString () << ", ";
//    }
//    std::cout << std::endl;

    /////////////////////////////////////////////////////////////////
    // perform the simulation
    /////////////////////////////////////////////////////////////////

    // temporary matrices
    size_t nrows = meshInit.getSpatialDim ();
    size_t ncols = meshInit.getNodesPerElem();

    Galois::Runtime::PerThreadStorage<LocalVec> perIterLocalVec;
    for (unsigned int i = 0; i < perIterLocalVec.size(); ++i)
      *perIterLocalVec.getRemote(i) = LocalVec(nrows, ncols);


    IterCounter iter;

    Process p (graph, inDegVec, meshInit, g, perIterLocalVec, createSyncFiles, iter);

    Galois::for_each(initWL.begin (), initWL.end (), p, Galois::wl<AVIWorkList>());

    printf ("iterations = %zd\n", iter.reduce ());

  }



};

#endif

