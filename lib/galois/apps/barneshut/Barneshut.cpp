/** Barnes-hut application -*- C++ -*-
 * @file
 * @section License
 *
 * Galois, a framework to exploit amorphous data-parallelism in irregular
 * programs.
 *
 * Copyright (C) 2012, The University of Texas at Austin. All rights reserved.
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
 * @author Martin Burtscher <burtscher@txstate.edu>
 * @author Donald Nguyen <ddn@cs.utexas.edu>
 */
#include "Galois/config.h"
#include "Galois/Galois.h"
#include "Galois/Statistic.h"
#include "Galois/Bag.h"
#include "llvm/Support/CommandLine.h"
#include "Lonestar/BoilerPlate.h"

#include <boost/math/constants/constants.hpp>
#include <boost/iterator/transform_iterator.hpp>

#include GALOIS_CXX11_STD_HEADER(array)
#include <limits>
#include <iostream>
#include <fstream>
#include <strings.h>
#include GALOIS_CXX11_STD_HEADER(deque)

#include "Point.h"

const char* name = "Barnshut N-Body Simulator";
const char* desc =
  "Simulates gravitational forces in a galactic cluster using the "
  "Barnes-Hut n-body algorithm";
const char* url = "barneshut";

static llvm::cl::opt<int> nbodies("n", llvm::cl::desc("Number of bodies"), llvm::cl::init(10000));
static llvm::cl::opt<int> ntimesteps("steps", llvm::cl::desc("Number of steps"), llvm::cl::init(1));
static llvm::cl::opt<int> seed("seed", llvm::cl::desc("Random seed"), llvm::cl::init(7));

struct Node {
  Point pos;
  double mass;
  bool Leaf;
};

struct Body : public Node {
  Point vel;
  Point acc;
};

/**
 * A node in an octree is either an internal node or a leaf.
 */
struct Octree : public Node {
  std::array<Galois::Runtime::LL::PtrLock<Node,true>, 8> child;
  char cLeafs;
  char nChildren;

  Octree(const Point& p) {
    Node::pos = p;
    Node::Leaf = false;
    cLeafs = 0;
    nChildren = 0;
  }
};

std::ostream& operator<<(std::ostream& os, const Body& b) {
  os << "(pos:" << b.pos
     << " vel:" << b.vel
     << " acc:" << b.acc
     << " mass:" << b.mass << ")";
  return os;
}

struct BoundingBox {
  Point min;
  Point max;
  explicit BoundingBox(const Point& p) : min(p), max(p) { }
  BoundingBox() :
    min(std::numeric_limits<double>::max()),
    max(std::numeric_limits<double>::min()) { }

  void merge(const BoundingBox& other) {
    min.pairMin(other.min);
    max.pairMax(other.max);
  }

  void merge(const Point& other) {
    min.pairMin(other);
    max.pairMax(other);
  }

  double diameter() const { return (max - min).minDim(); }
  double radius() const   { return diameter() * 0.5;     }
  Point center() const    { return (min + max) * 0.5;    }
};

std::ostream& operator<<(std::ostream& os, const BoundingBox& b) {
  os << "(min:" << b.min << " max:" << b.max << ")";
  return os;
}

struct Config {
  const double dtime; // length of one time step
  const double eps; // potential softening parameter
  const double tol; // tolerance for stopping recursion, <0.57 to bound error
  const double dthf, epssq, itolsq;
  Config():
    dtime(0.5),
    eps(0.05),
    tol(0.05), //0.025),
    dthf(dtime * 0.5),
    epssq(eps * eps),
    itolsq(1.0 / (tol * tol))  { }
};

std::ostream& operator<<(std::ostream& os, const Config& c) {
  os << "Barnes-Hut configuration:"
    << " dtime: " << c.dtime
    << " eps: " << c.eps
    << " tol: " << c.tol;
  return os;
}

Config config;

inline int getIndex(const Point& a, const Point& b) {
  int index = 0;
  for (int i = 0; i < 3; ++i)
    if (a[i] < b[i]) 
      index += (1 << i);
  return index;
}

inline Point updateCenter(Point v, int index, double radius) {
  for (int i = 0; i < 3; i++)
    v[i] += (index & (1 << i)) > 0 ? radius : -radius;
  return v;
}

typedef Galois::InsertBag<Body> Bodies;
typedef Galois::InsertBag<Body*> BodyPtrs;
//FIXME: reclaim memory for multiple steps
typedef Galois::InsertBag<Octree> Tree;

struct BuildOctree {
  Octree* root;
  Tree& T;
  double root_radius;

  BuildOctree(Octree* _root, Tree& _t, double radius) 
    : root(_root), T(_t), root_radius(radius) {  }

  void operator()(Body* b) { insert(b, root, root_radius); }

  void insert(Body* b, Octree* node, double radius) {
    int index = getIndex(node->pos, b->pos);
    Node* child = node->child[index].getValue();

    //go through the tree lock-free while we can
    if (child && !child->Leaf) {
      insert(b, static_cast<Octree*>(child), radius);
      return;
    }

    node->child[index].lock();
    child = node->child[index].getValue();
    
    if (child == NULL) {
      node->child[index].unlock_and_set(b);
      return;
    }
    
    radius *= 0.5;
    if (child->Leaf) {
      // Expand leaf
      Octree* new_node = &T.emplace(updateCenter(node->pos, index, radius));
      assert(node->pos != b->pos);
      //node->child[index].unlock_and_set(new_node);
      insert(b, new_node, radius);
      insert(static_cast<Body*>(child), new_node, radius);
      node->child[index].unlock_and_set(new_node);
    } else {
      node->child[index].unlock();
      insert(b, static_cast<Octree*>(child), radius);
    }
  }
};

unsigned computeCenterOfMass(Octree* node) {
  double mass = 0.0;
  Point accum;
  unsigned num = 1;

  //Reorganize leaves to be dense
  //remove copies values
  int index = 0;
  for (int i = 0; i < 8; ++i)
    if (node->child[i].getValue())
      node->child[index++].setValue(node->child[i].getValue());
  for (int i = index; i < 8; ++i)
    node->child[i].setValue(NULL);
  node->nChildren = index;

  for (int i = 0; i < index; i++) {
    Node* child = node->child[i].getValue();
    if (!child->Leaf) {
      num += computeCenterOfMass(static_cast<Octree*>(child));
    } else {
      node->cLeafs |= (1 << i);
      ++num;
    }
    mass += child->mass;
    accum += child->pos * child->mass;
  }
  
  node->mass = mass;
  
  if (mass > 0.0)
    node->pos = accum / mass;
  return num;
}

/*
void printRec(std::ofstream& file, Node* node, unsigned level) {
  static const char* ct[] = {
    "blue", "cyan", "aquamarine", "chartreuse", 
    "darkorchid", "darkorange", 
    "deeppink", "gold", "chocolate"
  };
  if (!node) return;
  file << "\"" << node << "\" [color=" << ct[node->owner / 4] << (node->owner % 4 + 1) << (level ? "" : " style=filled") << " label = \"" << (node->Leaf ? "L" : "N") << "\"];\n";
  if (!node->Leaf) {
    Octree* node2 = static_cast<Octree*>(node);
    for (int i = 0; i < 8 && node2->child[i]; ++i) {
      if (level == 3 || level == 6)
       	file << "subgraph cluster_" << level << "_" << i << " {\n";
      file << "\"" << node << "\" -> \"" << node2->child[i] << "\" [weight=0.01]\n";
      printRec(file, node2->child[i], level + 1);
      if (level == 3 || level == 6)
       	file << "}\n";
    }
  }
}

void printTree(Octree* node) {
  std::ofstream file("out.txt");
  file << "digraph octree {\n";
  file << "ranksep = 2\n";
  file << "root = \"" << node << "\"\n";
  //  file << "overlap = scale\n";
  printRec(file, node, 0);
  file << "}\n";
}
*/

Point updateForce(Point delta, double psq, double mass) {
  // Computing force += delta * mass * (|delta|^2 + eps^2)^{-3/2}
  double idr = 1 / sqrt((float) (psq + config.epssq));
  double scale = mass * idr * idr * idr;
  return delta * scale;
}

struct ComputeForces {
  // Optimize runtime for no conflict case
  typedef int tt_does_not_need_aborts;
  typedef int tt_needs_per_iter_alloc;
  typedef int tt_does_not_need_push;

  Octree* top;
  double diameter;
  double root_dsq;

  ComputeForces(Octree* _top, double _diameter) :
    top(_top),
    diameter(_diameter) {
    root_dsq = diameter * diameter * config.itolsq;
  }
  
  template<typename Context>
  void operator()(Body* b, Context& cnx) {
    Point p = b->acc;
    b->acc = Point(0.0, 0.0, 0.0);
    iterate(*b, root_dsq, cnx);
    b->vel += (b->acc - p) * config.dthf;
  }

  struct Frame {
    double dsq;
    Octree* node;
    Frame(Octree* _node, double _dsq) : dsq(_dsq), node(_node) { }
  };

  template<typename Context>
  void iterate(Body& b, double root_dsq, Context& cnx) {
    std::deque<Frame, Galois::PerIterAllocTy::rebind<Frame>::other> stack(cnx.getPerIterAlloc());
    stack.push_back(Frame(top, root_dsq));

    while (!stack.empty()) {
      const Frame f = stack.back();
      stack.pop_back();

      Point p = b.pos - f.node->pos;
      double psq = p.dist2();

      // Node is far enough away, summarize contribution
      if (psq >= f.dsq) {
        b.acc += updateForce(p, psq, f.node->mass);
        continue;
      }

      double dsq = f.dsq * 0.25;
      for (int i = 0; i < f.node->nChildren; i++) {
	Node* n = f.node->child[i].getValue();
	assert(n);
	if (f.node->cLeafs & (1 << i)) {
	  assert(n->Leaf);
	  if (static_cast<const Node*>(&b) != n) {
	    Point p = b.pos - n->pos;
	    b.acc += updateForce(p, p.dist2(), n->mass);
	  }
	} else {
#ifndef GALOIS_CXX11_DEQUE_HAS_NO_EMPLACE
	  stack.emplace_back(static_cast<Octree*>(n), dsq);
#else
	  stack.push_back(Frame(static_cast<Octree*>(n), dsq));
#endif
	  __builtin_prefetch(n);
	}
      }
    }
  }
};

struct AdvanceBodies {
  // Optimize runtime for no conflict case
  typedef int tt_does_not_need_aborts;

  AdvanceBodies() { }

  template<typename Context>
  void operator()(Body* b, Context&) {
    operator()(b);
  }

  void operator()(Body* b) {
    Point dvel(b->acc);
    dvel *= config.dthf;
    Point velh(b->vel);
    velh += dvel;
    b->pos += velh * config.dtime;
    b->vel = velh + dvel;
  }
};

struct ReduceBoxes {
  // NB: only correct when run sequentially or tree-like reduction
  typedef int tt_does_not_need_stats;
  BoundingBox initial;

  void operator()(const Body* b) {
    initial.merge(b->pos);
  }
};

struct mergeBox {
  void operator()(ReduceBoxes& lhs, ReduceBoxes& rhs) {
    return lhs.initial.merge(rhs.initial);
  }
};

double nextDouble() {
  return rand() / (double) RAND_MAX;
}

struct InsertBody {
  BodyPtrs& pBodies;
  Bodies& bodies;
  InsertBody(BodyPtrs& pb, Bodies& b): pBodies(pb), bodies(b) { }
  void operator()(const Body& b) {
    //Body b2 = b;
    //b2.owner = Galois::Runtime::LL::getTID();
    pBodies.push_back(&(bodies.push_back(b)));
  }
};

struct centerXCmp {
  template<typename T>
  bool operator()(const T& lhs, const T& rhs) const {
    return lhs.pos[0] < rhs.pos[0];
  }
};

struct centerYCmp {
  template<typename T>
  bool operator()(const T& lhs, const T& rhs) const {
    return lhs.pos[1] < rhs.pos[1];
  }
};

struct centerYCmpInv {
  template<typename T>
  bool operator()(const T& lhs, const T& rhs) const {
    return rhs.pos[1] < lhs.pos[1];
  }
};


template<typename Iter>
void divide(const Iter& b, const Iter& e) {
  if (std::distance(b,e) > 32) {
    std::sort(b,e, centerXCmp());
    Iter m = Galois::split_range(b,e);
    std::sort(b,m, centerYCmpInv());
    std::sort(m,e, centerYCmp());
    divide(b, Galois::split_range(b,m));
    divide(Galois::split_range(b,m), m);
    divide(m,Galois::split_range(m,e));
    divide(Galois::split_range(m,e), e);
  } else {
    std::random_shuffle(b,e);
  }
}

/**
 * Generates random input according to the Plummer model, which is more
 * realistic but perhaps not so much so according to astrophysicists
 */
void generateInput(Bodies& bodies, BodyPtrs& pBodies, int nbodies, int seed) {
  double v, sq, scale;
  Point p;
  double PI = boost::math::constants::pi<double>();

  srand(seed);

  double rsc = (3 * PI) / 16;
  double vsc = sqrt(1.0 / rsc);

  std::vector<Body> tmp;

  for (int body = 0; body < nbodies; body++) {
    double r = 1.0 / sqrt(pow(nextDouble() * 0.999, -2.0 / 3.0) - 1);
    do {
      for (int i = 0; i < 3; i++)
        p[i] = nextDouble() * 2.0 - 1.0;
      sq = p.dist2();
    } while (sq > 1.0);
    scale = rsc * r / sqrt(sq);

    Body b;
    b.mass = 1.0 / nbodies;
    b.pos = p * scale;
    do {
      p[0] = nextDouble();
      p[1] = nextDouble() * 0.1;
    } while (p[1] > p[0] * p[0] * pow(1 - p[0] * p[0], 3.5));
    v = p[0] * sqrt(2.0 / sqrt(1 + r * r));
    do {
      for (int i = 0; i < 3; i++)
        p[i] = nextDouble() * 2.0 - 1.0;
      sq = p.dist2();
    } while (sq > 1.0);
    scale = vsc * v / sqrt(sq);
    b.vel = p * scale;
    b.Leaf = true;
    tmp.push_back(b);
    //pBodies.push_back(&bodies.push_back(b));
  }

  //sort and copy out
  divide(tmp.begin(), tmp.end());
  Galois::do_all(tmp.begin(), tmp.end(), InsertBody(pBodies, bodies));
}

struct CheckAllPairs {
  Bodies& bodies;
  
  CheckAllPairs(Bodies& b): bodies(b) { }

  double operator()(const Body& body) {
    const Body* me = &body;
    Point acc;
    for (Bodies::iterator ii = bodies.begin(), ei = bodies.end(); ii != ei; ++ii) {
      Body* b = &*ii;
      if (me == b)
        continue;
      Point delta = me->pos - b->pos;
      double psq = delta.dist2();
      acc += updateForce(delta, psq, b->mass);
    }

    double dist2 = acc.dist2();
    acc -= me->acc;
    double retval = acc.dist2() / dist2;
    return retval;
  }
};

double checkAllPairs(Bodies& bodies, int N) {
  Bodies::iterator end(bodies.begin());
  std::advance(end, N);
  
  return Galois::ParallelSTL::map_reduce(bodies.begin(), end,
      CheckAllPairs(bodies),
      0.0,
      std::plus<double>()) / N;
}

void run(Bodies& bodies, BodyPtrs& pBodies) {
  typedef Galois::WorkList::dChunkedLIFO<256> WL_;
  typedef Galois::WorkList::AltChunkedLIFO<32> WL;
  typedef Galois::WorkList::StableIterator<decltype(pBodies.local_begin()), true> WLL;

  for (int step = 0; step < ntimesteps; step++) {
    // Do tree building sequentially
    BoundingBox box = Galois::Runtime::do_all_impl(Galois::Runtime::makeLocalRange(pBodies), ReduceBoxes(), mergeBox(), "reduceBoxes", true).initial;
    //std::for_each(bodies.begin(), bodies.end(), ReduceBoxes(box));

    Tree t;
    Octree& top = t.emplace(box.center());

    Galois::StatTimer T_build("BuildTime");
    T_build.start();
    Galois::do_all_local(pBodies, BuildOctree(&top, t, box.radius()), Galois::loopname("BuildTree"));
    T_build.stop();

    //update centers of mass in tree
    unsigned size = computeCenterOfMass(&top);
    //printTree(&top);
    std::cout << "Tree Size: " << size << "\n";

    Galois::StatTimer T_compute("ComputeTime");
    T_compute.start();
    Galois::for_each_local(pBodies, ComputeForces(&top, box.diameter()), Galois::loopname("compute"), Galois::wl<WLL>());
    T_compute.stop();

    if (!skipVerify) {
      std::cout << "MSE (sampled) " << checkAllPairs(bodies, std::min((int) nbodies, 100)) << "\n";
    }
    //Done in compute forces
    Galois::do_all_local(pBodies, AdvanceBodies(), Galois::loopname("advance"));

    std::cout << "Timestep " << step << " Center of Mass = ";
    std::ios::fmtflags flags = 
      std::cout.setf(std::ios::showpos|std::ios::right|std::ios::scientific|std::ios::showpoint);
    std::cout << top.pos;
    std::cout.flags(flags);
    std::cout << "\n";
  }
}

int main(int argc, char** argv) {
  Galois::StatManager M;
  LonestarStart(argc, argv, name, desc, url);

  std::cout << config << "\n";
  std::cout << nbodies << " bodies, "
            << ntimesteps << " time steps\n";

  Bodies bodies;
  BodyPtrs pBodies;
  generateInput(bodies, pBodies, nbodies, seed);

  Galois::StatTimer T;
  T.start();
  run(bodies, pBodies);
  T.stop();
}
