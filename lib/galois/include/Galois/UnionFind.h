/** Union-find -*- C++ -*-
 * @file
 *
 * A minimum spanning tree algorithm to demonstrate the Galois system.
 *
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
 * @author Donald Nguyen <ddn@cs.utexas.edu>
 */
#ifndef GALOIS_UNIONFIND_H
#define GALOIS_UNIONFIND_H

namespace Galois {
/**
 * Intrusive union-find implementation. Users subclass this to get disjoint
 * functionality for the subclass object.
 */
template<typename T>
class UnionFindNode {
  T* findImpl() const {
    if (isRep()) return m_component;

    T* rep = m_component;
    while (rep->m_component != rep) {
      T* next = rep->m_component;
      rep = next;
    }
    return rep;
  }

protected:
  T* m_component;

  UnionFindNode(): m_component(reinterpret_cast<T*>(this)) { }

public:
  typedef UnionFindNode<T> SuperTy;

  bool isRep() const {
    return m_component == this;
  }

  const T* find() const { return findImpl(); }

  T* find() { return findImpl(); }

  T* findAndCompress() {
    // Basic outline of race in synchronous path compression is that two path
    // compressions along two different paths to the root can create a cycle
    // in the union-find tree. Prevent that from happening by compressing
    // incrementally.
    if (isRep()) return m_component;

    T* rep = m_component;
    T* prev = 0;
    while (rep->m_component != rep) {
      T* next = rep->m_component;

      if (prev && prev->m_component == rep) {
        prev->m_component = next;
      }
      prev = rep;
      rep = next;
    }
    return rep;
  }

  //! Lock-free merge. Returns if merge was done.
  T* merge(T* b) {
    T* a = m_component;
    while (true) {
      a = a->findAndCompress();
      b = b->findAndCompress();
      if (a == b)
        return 0;
      // Avoid cycles by directing edges consistently
      if (a > b)
        std::swap(a, b);
      if (__sync_bool_compare_and_swap(&a->m_component, a, b)) {
        return b;
      }
    }
  }
};
}
#endif
