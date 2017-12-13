/** Survey propagation -*- C++ -*-
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
 * @section Description
 *
 * Survey Propagation
 *
 * @author Andrew Lenharth <andrewl@lenharth.org>
 */
#include "Galois/Statistic.h"
#include "Galois/Graph/Graph.h"
#include "Galois/Galois.h"
#include "Galois/Accumulator.h"

#include "llvm/Support/CommandLine.h"

#include "Lonestar/BoilerPlate.h"

#ifdef GALOIS_USE_EXP
#include "Galois/PriorityScheduling.h"
#endif

#include <cstdlib>
#include <iostream>

#include <math.h>

using namespace std;
using namespace Galois::WorkList;

namespace cll = llvm::cl;

static const char* name = "Survey Propagation";
static const char* desc = "Solves SAT problems using survey propagation";
static const char* url = "survey_propagation";

static cll::opt<int> seed(cll::Positional, cll::desc("<seed>"), cll::Required);
static cll::opt<int> M(cll::Positional, cll::desc("<num variables>"), cll::Required);
static cll::opt<int> N(cll::Positional, cll::desc("<num clauses>"), cll::Required);
static cll::opt<int> K(cll::Positional, cll::desc("<variables per clause>"), cll::Required);

//#define WLWL LocalQueues<ChunkedFIFO<1024>, FIFO<> >
#define WLWL dChunkedFIFO<1024>

//SAT problem:
//variables Xi E {0,1}, i E {1 .. N), M constraints
//constraints are or clauses of variables or negation of variables
//clause a has variables i1...iK, J^a_ir E {-+1}
//zi1 = J^a_ir * xir

//Graph form:
//N variables each get a variable node (circles in paper) (SET X, i,j,k...)
//M clauses get a function node (square in paper) (set A, a,b,c...)
// edge between xi and a if xi appears in a, weighted by J^a_i
//V(i) function nodes a... to which variable node i is connected
//n_i = |V(i)| = degree of variable node
//V+(i) positive edges, V-(i) negative edges (per J^a_i) (V(i) = V+(i) + V-(i))
//V(i)\b set V(i) without b
//given connected Fnode a and Vnode j, V^u_a(j) and V^s_a(j) are neighbors which cause j sat or unsat a:
// if (J^a_j = 1): V^u_a(j) = V+(j); V^s_a(j) = V-(j)\a
// if (J^a_j = -1): V^u_a(j) = V-(j); V^s_a(j) = V+(j)\a

//Graph+data:
//survey n_a->i E [0,1]



//implementation
//As a graph
//nodes have:
//  a name
//  a eta product
//edges have:
//  a double survey
//  a bool for sign (inversion of variable)
//  a pi product
//Graph is undirected (for now)

struct SPEdge {
  double eta;
  bool isNegative;

  SPEdge() {}
  SPEdge(bool isNeg) :isNegative(isNeg) {
    eta = (double)rand() / (double)RAND_MAX;
  }
};

struct SPNode {
  bool isClause;
  int name;
  bool solved;
  bool value;
  int t;
  
  double Bias;

  SPNode(int n, bool b) :isClause(b), name(n), solved(false), value(false), t(0) {}
};

typedef Galois::Graph::FirstGraph<SPNode, SPEdge, false> Graph;
typedef Galois::Graph::FirstGraph<SPNode, SPEdge, false>::GraphNode GNode;

static Graph graph;

static std::vector<GNode> literals;
static std::vector<std::pair<GNode,int> > clauses;

static Galois::GAccumulator<unsigned int> nontrivial;

static Galois::GReduceMax<double> maxBias;
static Galois::GAccumulator<int> numBias;
static Galois::GAccumulator<double> sumBias;

//interesting parameters:
static const double epsilon = 0.000001;
static const int tmax = 100;
//static int tlimit = 0;

void initialize_random_formula(int M, int N, int K) {
  //M clauses
  //N variables
  //K vars per clause

  //build up clauses and literals
  clauses.resize(M);
  literals.resize(N);

  for (int m = 0; m < M; ++m) {
    GNode node = graph.createNode(SPNode(m, true));
    graph.addNode(node, Galois::MethodFlag::NONE);
    clauses[m] = std::make_pair(node, 0);
  }
  for (int n = 0; n < N; ++n) {
    GNode node = graph.createNode(SPNode(n, false));
    graph.addNode(node, Galois::MethodFlag::NONE);
    literals[n] = node;
  }

  for (int m = 0; m < M; ++m) {
    //Generate K unique values
    std::vector<int> touse;
    while (touse.size() != (unsigned)K) {
      //extra complex to generate uniform rand value
      int newK = (int)(((double)rand()/((double)RAND_MAX + 1)) * (double)(N));
      if (std::find(touse.begin(), touse.end(), newK) == touse.end()) {
	touse.push_back(newK);
	graph.getEdgeData(graph.addEdge(clauses[m].first, literals[newK], Galois::MethodFlag::NONE)) = SPEdge((bool)(rand() % 2));
      }
    }
  }

  //std::random_shuffle(literals.begin(), literals.end());
  //std::random_shuffle(clauses.begin(), clauses.end());
}

void print_formula() {
  for (unsigned m = 0; m < clauses.size(); ++m) {
    if (m != 0)
      std::cout << " & ";
    std::cout << "c" << m << "( ";
    GNode N = clauses[m].first;
    for (Graph::edge_iterator ii = graph.edge_begin(N, Galois::MethodFlag::NONE), ee = graph.edge_end( N, Galois::MethodFlag::NONE); ii != ee; ++ii) {
      if (ii != graph.edge_begin(N, Galois::MethodFlag::NONE))
	std::cout << " | ";
      SPEdge& E = graph.getEdgeData(ii, Galois::MethodFlag::NONE);
      if (E.isNegative)
	std::cout << "-";
      
      SPNode& V = graph.getData(graph.getEdgeDst(ii), Galois::MethodFlag::NONE);
      std::cout << "v" << V.name;
      if (V.solved)
	std::cout << "[" << (V.value ? 1 : 0) << "]";
      std::cout << "{" << E.eta << "," << V.Bias << "," << (V.value ? 1 : 0) << "}";
      std::cout << " ";
    }
    std::cout << " )";
  }
  std::cout << "\n";
}

void print_fixed() {
  for (unsigned n = 0; n < literals.size(); ++n) {
    GNode N = literals[n];
    SPNode& V = graph.getData(N, Galois::MethodFlag::NONE);
    if (V.solved)
      std::cout << V.name << "[" << (V.value ? 1 : 0) << "] ";
  }
  std::cout << "\n";
}

int count_fixed() {
  int retval = 0;
  for (unsigned n = 0; n < literals.size(); ++n) {
    GNode N = literals[n];
    SPNode& V = graph.getData(N, Galois::MethodFlag::NONE);
    if (V.solved)
      ++retval;
  }
  return retval;
}

struct update_eta {

  double eta_for_a_i(GNode a, GNode i) {
    double etaNew = 1.0;
    //for each j
    for (Graph::edge_iterator jii = graph.edge_begin(a, Galois::MethodFlag::NONE), 
	   jee = graph.edge_end(a, Galois::MethodFlag::NONE); jii != jee; ++jii) {
      GNode j = graph.getEdgeDst(jii);
      if (j != i) {
	bool ajNegative = graph.getEdgeData(jii, Galois::MethodFlag::NONE).isNegative;
	double prodP = 1.0;
	double prodN = 1.0;
	double prod0 = 1.0;
	//for each b
	for (Graph::edge_iterator bii = graph.edge_begin(j, Galois::MethodFlag::NONE), 
	       bee = graph.edge_end(j, Galois::MethodFlag::NONE); 
	     bii != bee; ++bii) {
	  GNode b = graph.getEdgeDst(bii);
	  SPEdge Ebj = graph.getEdgeData(bii, Galois::MethodFlag::NONE);
	  if (b != a)
	    prod0 *= (1.0 - Ebj.eta);
	  if (Ebj.isNegative)
	    prodN *= (1.0 - Ebj.eta);
	  else
	    prodP *= (1.0 - Ebj.eta);
	}
	double PIu, PIs;
	if (ajNegative) {
	  PIu = (1.0 - prodN) * prodP;
	  PIs = (1.0 - prodP) * prodN;
	} else {
	  PIs = (1.0 - prodN) * prodP;
	  PIu = (1.0 - prodP) * prodN;
	}
	double PI0 = prod0;
	etaNew *= (PIu / (PIu + PIs + PI0));
      }
    }
    return etaNew;
  }

  template<typename Context>
  void operator()(std::pair<GNode,int> a, Context& ctx) {
    this->operator()(a.first, ctx);
  }

  template<typename Context>
  void operator()(GNode a, Context& ctx) {
    //std::cerr << graph.getData(a).t << " ";
    // if (graph.getData(a, Galois::MethodFlag::NONE).t >= tlimit)
    //   return;

    //    for (Graph::neighbor_iterator iii = graph.neighbor_begin(a), 
    //      	   iee = graph.neighbor_end(a); iii != iee; ++iii)
    //   for (Graph::neighbor_iterator bii = graph.neighbor_begin(*iii), 
    // 	     bee = graph.neighbor_end(*iii); bii != bee; ++bii)
    // 	//	for (Graph::neighbor_iterator jii = graph.neighbor_begin(*bii), 
    // 	//     	       jee = graph.neighbor_end(*bii); 
    // 	//     	     jii != jee; ++jii)
    //{}

    ++graph.getData(a).t;

    //for each i
    for (Graph::edge_iterator iii = graph.edge_begin(a, Galois::MethodFlag::NONE), 
	   iee = graph.edge_end(a, Galois::MethodFlag::NONE); iii != iee; ++iii) {
      GNode i = graph.getEdgeDst(iii);
      double e = eta_for_a_i(a, i);
      double olde = graph.getEdgeData(iii, Galois::MethodFlag::NONE).eta;
      graph.getEdgeData(iii).eta = e;
      //std::cout << olde << ',' << e << " ";
      if (fabs(olde - e) > epsilon) {
	for (Graph::edge_iterator bii = graph.edge_begin(i, Galois::MethodFlag::NONE), 
	   bee = graph.edge_end(i, Galois::MethodFlag::NONE); bii != bee; ++bii) {
	  GNode b = graph.getEdgeDst(bii);
	  if (a != b) // && graph.getData(b, Galois::MethodFlag::NONE).t < tlimit)
	    ctx.push(std::make_pair(b,100-(int)(100.0*(olde - e))));
	}
      }
    }
  }
};

//compute biases on each node
struct update_biases {
  void operator()(GNode i) {
    SPNode& idata = graph.getData(i, Galois::MethodFlag::NONE);
    if (idata.solved) return;

    double pp1 = 1.0;
    double pp2 = 1.0;
    double pn1 = 1.0;
    double pn2 = 1.0;
    double p0 = 1.0;

    //for each function a
    for (Graph::edge_iterator aii = graph.edge_begin(i, Galois::MethodFlag::NONE), aee = graph.edge_end(i, Galois::MethodFlag::NONE); aii != aee; ++aii) {
      SPEdge& aie = graph.getEdgeData(aii, Galois::MethodFlag::NONE);

      double etaai = aie.eta;
      if (etaai > epsilon)
	nontrivial += 1;
      if (aie.isNegative) {
	pp2 *= (1.0 - etaai);
	pn1 *= (1.0 - etaai);
      } else {
	pp1 *= (1.0 - etaai);
	pn2 *= (1.0 - etaai);
      }
      p0 *= (1.0 - etaai);
    }
    double pp = (1.0 - pp1) * pp2;
    double pn = (1.0 - pn1) * pn2;
    
    double BiasP = pp / (pp + pn + p0);
    double BiasN = pn / (pp + pn + p0);
    //    double Bias0 = 1.0 - BiasP - BiasN;

    double d = BiasP - BiasN;
    if (d < 0.0)
      d = BiasN - BiasP;
    idata.Bias = d;
    idata.value = (BiasP > BiasN);

    assert(!std::isnan(d) && !std::isnan(-d));
    maxBias.update(d);
    numBias += 1;
    sumBias += d;
  }
};

struct EIndexer: public std::unary_function<std::pair<GNode,int>,int> {
  int operator()(const std::pair<GNode,int>& v) {
    return v.second;
  }
};

struct ELess {
  bool operator()(const std::pair<GNode,int>& lhs, 
		  const std::pair<GNode,int>& rhs) {
    if (lhs.second < rhs.second) return true;
    if (lhs.second > rhs.second) return false;
    return lhs < rhs;
  }
};
struct EGreater {
  bool operator()(const std::pair<GNode,int>& lhs, 
		  const std::pair<GNode,int>& rhs) {
    if (lhs.second > rhs.second) return true;
    if (lhs.second < rhs.second) return false;
    return lhs > rhs;
  }
};

//return true if converged
void SP_algorithm() {
  //0) at t = 0, for every edge a->i, randomly initialize the message sigma a->i(t=0) in [0,1]
  //1) for t = 1 to tmax:
  //1.1) sweep the set of edges in a random order, and update sequentially the warnings on all the edges of the graph, generating the values sigma a->i (t) using SP_update
  //1.2) if (|sigma a->i(t) - sigma a->i (t-1) < E on all the edges, the iteration has converged and generated sigma* a->i = sigma a->i(t), goto 2
  //2) if t = tmax return un-converged.  if (t < tmax) then return the set of fixed point warnings sigma* a->i = sigma a->i (t)
  
  //  tlimit += tmax;
#ifdef GALOIS_USE_EXP
  Exp::PriAuto<64, EIndexer, WLWL, ELess, EGreater >::for_each(clauses.begin(), clauses.end(), update_eta(), "update_eta");
#else
  Galois::for_each(clauses.begin(), clauses.end(), update_eta(), Galois::loopname("update_eta"), Galois::wl<WLWL>());
#endif

  maxBias.reset();
  numBias.reset();
  sumBias.reset();
  nontrivial.reset();
  Galois::do_all(literals.begin(), literals.end(), update_biases(), Galois::loopname("update_biases"));
}

struct fix_variables {
  double limit;
  fix_variables(double d) :limit(d) {}
  void operator()(GNode i) {//, const Context& ctx) {
    SPNode& idata = graph.getData(i);
    if (idata.solved) return;
    if (idata.Bias > limit) {
      idata.solved = true;
      //TODO: simplify graph
      //for each b
      for (Graph::edge_iterator bii = graph.edge_begin(i), 
	     bee = graph.edge_end(i); 
	   bii != bee; ++bii) {
	graph.getData(graph.getEdgeDst(bii)).solved = true;
	graph.getData(graph.getEdgeDst(bii)).value = true;
      }
      graph.removeNode(i);
    }
  }
};

void decimate() {
  double m = maxBias.reduce();
  double n = nontrivial.reduce();
  int num = numBias.reduce();
  double average = num > 0 ? sumBias.reduce() / num : 0.0;
  std::cout << "NonTrivial " << n << " MaxBias " << m << " Average Bias " << average << "\n";
  double d = ((m - average) * 0.25) + average;
  Galois::do_all(literals.begin(), literals.end(), fix_variables(d), Galois::loopname("fix_variables"));
}

bool survey_inspired_decimation() {
  //0) Randomize initial conditions for the surveys
  //1) run SP
  //   if (SP does not converge, return SP UNCONVEREGED and stop
  //   if SP convereges, use fixed-point surveys n*a->i to
  //2) decimate
  //2.1) if non-trivial surveys (n != 0) are found, then:
  //   a) compute biases (W+,W-,W0) from PI+,PI-,PI0
  //   b) fix largest |W+ - W-| to x =  W+ > W-
  //   c) clean the graph
  //2.2) if all surveys are trivial(n = 0), output simplified subformula
  //4) if solved, output SAT, if no contradiction, continue at 1, if contridiction, stop
  do {
    SP_algorithm();
    if (nontrivial.reduce()) {
      std::cout << "DECIMATED\n";
      decimate();
    } else {
      std::cout << "SIMPLIFIED\n";
      return false;
    }
  } while (true); // while (true);
  return true;
}


int main(int argc, char** argv) {
  Galois::StatManager MM;
  LonestarStart(argc, argv, name, desc, url);
  srand(seed);
  initialize_random_formula(M,N,K);
  //print_formula();
  //build_graph();
  //print_graph();

  std::cout << "Starting...\n";

  Galois::StatTimer T;
  T.start();
  survey_inspired_decimation();
  T.stop();

  //print_formula();
  //print_fixed();

  std::cout << "Fixed " << count_fixed() << " variables\n";

  return 0;
}
