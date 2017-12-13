/** Simple STL style algorithms -*- C++ -*-
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
 * @author Andrew Lenharth <andrewl@lenharth.org>
 */
#ifndef GALOIS_GSTL_H
#define GALOIS_GSTL_H

#include <algorithm>
#include <iterator>
#include <utility>

namespace Galois {

template<typename IterTy, class Distance>
IterTy safe_advance_dispatch(IterTy b, IterTy e, Distance n, std::random_access_iterator_tag) {
  if (std::distance(b,e) < n)
    return b + n;
  else
    return e;
}

template<typename IterTy, class Distance>
IterTy safe_advance_dispatch(IterTy b, IterTy e, Distance n, std::input_iterator_tag) {
  while (b != e && n--)
    ++b;
  return b;
}

/**
 * Like std::advance but returns end if end is closer than the advance amount.
 */
template<typename IterTy, class Distance>
IterTy safe_advance(IterTy b, IterTy e, Distance n) {
  typename std::iterator_traits<IterTy>::iterator_category category;
  return safe_advance_dispatch(b,e,n,category);
}


/**
 * Finds the midpoint of a range.  The first half is always be bigger than
 * the second half if the range has an odd length.
 */
template<typename IterTy>
IterTy split_range(IterTy b, IterTy e) {
  std::advance(b, (std::distance(b,e) + 1) / 2);
  return b;
}

/**
 * Returns a continuous block from the range based on the number of
 * divisions and the id of the block requested
 */
template<typename IterTy>
std::pair<IterTy, IterTy> block_range(IterTy b, IterTy e, unsigned id, unsigned num) {
  unsigned int dist = std::distance(b, e);
  unsigned int numper = std::max((dist + num - 1) / num, 1U); //round up
  unsigned int A = std::min(numper * id, dist);
  unsigned int B = std::min(numper * (id + 1), dist);
  std::advance(b, A);
  if (dist != B) {
    e = b;
    std::advance(e, B - A);
  }
  return std::make_pair(b,e);
}


//! Destroy a range
template<class InputIterator>
void uninitialized_destroy ( InputIterator first, InputIterator last )
{
  typedef typename std::iterator_traits<InputIterator>::value_type T;
  for (; first!=last; ++first)
    (&*first)->~T();
}

}
#endif
