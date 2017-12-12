#ifndef GALOIS_GRAPHNODEBAG_H
#define GALOIS_GRAPHNODEBAG_H

#include "Galois/Accumulator.h"
#include "Galois/LargeArray.h"
#include "Galois/Bag.h"

namespace Galois {

/**
 * Stores graph nodes to execute for {@link Ligra} executor.
 */
template<unsigned int BlockSize = 0>
class GraphNodeBag {
  typedef Galois::InsertBag<size_t, BlockSize> Bag;
  typedef Galois::LargeArray<bool> Bitmask;

  Bag bag;
  Galois::GAccumulator<size_t> counts;
  Galois::GAccumulator<size_t> numNodes;
  Bitmask bitmask;
  size_t size;
  bool isDense;

  struct InitializeSmall {
    GraphNodeBag* self;
    void operator()(size_t n) {
      self->bitmask[n] = 0;
    }
  };

  struct InitializeBig {
    GraphNodeBag* self;
    void operator()(unsigned id, unsigned total) {
      typedef typename Bitmask::iterator It;
      std::pair<It,It> p = Galois::block_range(self->bitmask.begin(), self->bitmask.end(), id, total);
      std::fill(p.first, p.second, 0);
    }
  };

  struct Densify {
    GraphNodeBag* self;
    void operator()(size_t n) {
      self->bitmask[n] = true;
    }
  };
public:
  GraphNodeBag(size_t n): size(n), isDense(false) { }

  typedef typename Bag::iterator iterator;
  typedef typename Bag::local_iterator local_iterator;

  iterator begin() { return bag.begin(); }
  iterator end() { return bag.end(); }
  local_iterator local_begin() { return bag.local_begin(); }
  local_iterator local_end() { return bag.local_end(); }

  void pushDense(size_t n, size_t numEdges) {
    assert(isDense);

    if (!bitmask[n]) {
      bitmask[n] = true;
      push(n, numEdges);
    }
  }

  void push(size_t n, size_t numEdges) {
    bag.push(n);
    numNodes += 1;
    counts += 1 + numEdges;
  }

  size_t getCount() { return counts.reduce(); }

  size_t getSize() { return numNodes.reduce(); }

  void clear() { 
    if (isDense) {
      if (numNodes.reduce() < bitmask.size() / 4) {
        InitializeSmall fn = { this };
        Galois::do_all_local(bag, fn);
      } else {
        InitializeBig fn = { this };
        Galois::on_each(fn);
      }
    }
    bag.clear();
    counts.reset();
    numNodes.reset();

    isDense = false;
  }

  bool contains(size_t n) {
    assert(isDense);
    return bitmask[n];
  }

  bool empty() const { return bag.empty(); }

  void densify() {
    isDense = true;
    if (bitmask.size() == 0) {
      bitmask.create(size);
    }

    Densify fn = { this };
    Galois::do_all_local(bag, fn);
  }
};

/**
 * Stores graph nodes to execute for {@link Ligra} executor. Unlike {@link
 * GraphNodeBag}, this class stores two bags to facilitate bulk-synchronous
 * processing.
 */
template<unsigned int BlockSize = 0>
class GraphNodeBagPair {
  GraphNodeBag<BlockSize> bag1;
  GraphNodeBag<BlockSize> bag2;
  int curp;
public:
  typedef GraphNodeBag<BlockSize> bag_type;

  GraphNodeBagPair(size_t n): bag1(n), bag2(n), curp(0) { }

  GraphNodeBag<BlockSize>& cur() { return (*this)[curp]; }
  GraphNodeBag<BlockSize>& next() { return (*this)[(curp+1) & 1]; }
  void swap() {
    curp = (curp + 1) & 1; 
    next().clear();
  }
  GraphNodeBag<BlockSize>& operator[](int i) {
    if (i == 0)
      return bag1;
    else
      return bag2;
  }
};

}
#endif
