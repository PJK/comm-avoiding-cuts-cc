/** Two Level Iterator for Per-thread workList-*- C++ -*-
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
 * Two Level Iterator for per-thread workList.
 *
 * Assumptions
 * <ul>
 *  <li>Outer and Inner iterators are default- and copy-constructible</li>
 *  <li>Inner and Outer must be at least forward_iterator_tag</li>
 *  <li>InnerBegFn and InnerEndFn take an argument of type *Outer and return an Inner
 *    pointing to begin or end of inner range.</li>
 *  <li>InnerBegFn and InnerEndFn must inherit from std::unary_function so that
 *    argument_type and result_type are available.</li>
 * </ul>
 *
 * Important pitfalls to handle
 * <ol>
 *  <li>If Outer and Inner have different categories, which category to choose?. The
 *  category of Inner can be chosen after (expensively) supporting moving backwards for 
 *  outer iterators of forward category. Note: Lowest category currently supported
 *  is forward iterators.</li>
 *
 *  <li>Prevent Outer from falling outside the [begin,end) range, because calling
 *  container functions e.g. outer->begin () and outer->end () is not valid and may
 *  cause a segfault.</li>
 *
 *  <li>The initial position of Outer and Inner iterators must be such that calling
 *  operator * or operator -> on at two level iterator yields a valid result (if
 *  possible). This means advancing the inner iterator to begin of first non-empty
 *  container (else to the end of outer). If the outer iterator is initialized to
 *  end of the outer range i.e. [end, end), then inner iterator cannot be
 *  initialized.</li>
 *
 *  <li>When incrementing (++), the inner iterator should initially be at a valid
 *  begin position, but after incrementing may end up at end of an Inner range.
 *  So the next valid local begin must be found, else the end of 
 *  outer should be reached</li>
 *
 *  <ol>
 *    <li> When jumping forward, outer should not go beyond end. After jump is
 *    completed, inner may be at local end, so a valid next begin must be found
 *    or else end of outer must be reached</li>
 *  </ol>
 *
 *  <li>When decrementing (--), the inner iterator may initially be uninitialized
 *  due to outer being at end (See 3 above).
 *  Inner iterator must be brought to a valid location after decrementing, or, else
 *  the begin of outer must be reached (and not exceeded).</li>
 *  
 *  <ol>
 *    <li>When jumping backward, inner iterator may be uninitialized due to
 *    outer being at end.</li>
 *  </ol>
 *
 *  <li>When jumping forward or backward, check for jump amount being negative.</li>
 *  <ol>
 *    <li>Jumping outside the range of outer cannot be supported.</li>
 *  </ol>
 *
 * </ol>
 *
 * @author <ahassaan@ices.utexas.edu>
 */

#ifndef GALOIS_TWO_LEVEL_ITER_H
#define GALOIS_TWO_LEVEL_ITER_H

#include "Galois/config.h"

#include <iterator>
#include GALOIS_CXX11_STD_HEADER(functional)
#include GALOIS_CXX11_STD_HEADER(type_traits)

#include <cstdlib>
#include <cassert>

namespace Galois {

namespace TwoLevelIteratorImpl {
  template <typename Iter>
  void safe_decrement (Iter& it, const Iter& beg, const Iter& end
      , std::forward_iterator_tag) {

    Iter next = beg;
    Iter curr (next);

    while (next != it) {
      curr = next;
      assert (next != end);
      ++next;
    }
    
    assert (next == it);
    assert (curr != it);

    it = curr;
  }

  template <typename Iter>
  void safe_decrement (Iter& it, const Iter& beg, const Iter& end
      , std::bidirectional_iterator_tag) {
    assert (it != beg);
    --it;
  }

  template <typename Iter>
  void safe_decrement (Iter& it, const Iter& beg, const Iter& end) {
    safe_decrement (it, beg, end
        , typename std::iterator_traits<Iter>::iterator_category ());
  }
}

//! Common functionality of TwoLevelIterators
template <typename Outer, typename Inner, typename InnerBegFn, typename InnerEndFn>
class TwoLevelIterBase {

protected:
  // TODO: make begin and end const
  Outer m_beg_outer;
  Outer m_end_outer;
  Outer m_outer;
  Inner m_inner;

  InnerBegFn innerBegFn;
  InnerEndFn innerEndFn;


  inline bool outerAtBegin () const {
    return m_outer == m_beg_outer;
  }

  inline bool outerAtEnd () const {
    return m_outer == m_end_outer;
  }

  inline bool outerEmpty () const {
    return m_beg_outer == m_end_outer;
  }

  inline Inner innerBegin () { 
    assert (!outerEmpty ());
    assert (!outerAtEnd ());

    return innerBegFn (*m_outer);
  }

  inline Inner innerEnd () {
    assert (!outerEmpty ());
    assert (!outerAtEnd ());

    return innerEndFn (*m_outer);
  }

  inline bool innerAtBegin () const {
    return m_inner == const_cast<TwoLevelIterBase*> (this)->innerBegin ();
  }

  inline bool innerAtEnd () const {
    return m_inner == const_cast<TwoLevelIterBase*> (this)->innerEnd ();
  }

  TwoLevelIterBase (): 
    m_beg_outer (), 
    m_end_outer (), 
    m_outer (), 
    m_inner (),
    innerBegFn (),
    innerEndFn ()
  {}

  TwoLevelIterBase (
      Outer beg_outer, 
      Outer end_outer,
      InnerBegFn innerBegFn,
      InnerEndFn innerEndFn)
    : 
      m_beg_outer (beg_outer), 
      m_end_outer (end_outer), 
      m_outer (m_beg_outer),
      m_inner (),
      innerBegFn (innerBegFn),
      innerEndFn (innerEndFn)
  {}

};


//! Two-Level forward iterator
template <typename Outer, typename Inner, typename InnerBegFn, typename InnerEndFn>
class TwoLevelFwdIter: 
  public std::iterator_traits<Inner>,
  public TwoLevelIterBase<Outer, Inner, InnerBegFn, InnerEndFn> {

protected:

  typedef std::iterator_traits<Inner> Traits;
  typedef TwoLevelIterBase<Outer, Inner, InnerBegFn, InnerEndFn> Base;


  void nextOuter () {
    assert (!Base::outerAtEnd ());
    assert (!Base::outerEmpty ());
    ++Base::m_outer;
    if (!Base::outerAtEnd ()) {
      Base::m_inner = Base::innerBegin ();
    }
  }

  void seekValidBegin () {
    while (!Base::outerAtEnd () && Base::innerAtEnd ()) {
      nextOuter ();
    } 
  }


  void step_forward () {
    assert (!Base::innerAtEnd ());
    ++Base::m_inner;

    if (Base::innerAtEnd ()) {
      seekValidBegin ();
    }
  }

  bool is_equal (const TwoLevelFwdIter& that) const {
    // the outer iterators of 'this' and 'that' have been initialized 
    // with either (beg,end), or, (end, end)
    //  - for two level begin, outer is initialized to (beg,end)
    //  - for two level end, outer is initialized to (end, end)
    assert (this->m_end_outer == that.m_end_outer);

    return (this->m_outer == that.m_outer) 
      && (Base::outerAtEnd () || (this->m_inner == that.m_inner));
  }


public:

  TwoLevelFwdIter (): Base () {}

  TwoLevelFwdIter (
      Outer beg_outer, 
      Outer end_outer,
      InnerBegFn innerBegFn,
      InnerEndFn innerEndFn)
    : 
      Base (beg_outer, end_outer, innerBegFn, innerEndFn) 
  {

    if (!Base::outerAtEnd ()) {
      Base::m_inner = Base::innerBegin ();
      seekValidBegin ();
    }
  }

  typename Traits::reference operator * () const { 
    return *Base::m_inner;
  }

  typename Traits::pointer operator -> () const {
    return Base::m_inner->operator -> ();
  }

  TwoLevelFwdIter& operator ++ () {
    step_forward ();
    return *this;
  }

  TwoLevelFwdIter operator ++ (int) {
    TwoLevelFwdIter tmp (*this);
    step_forward ();
    return tmp;
  }
    
  friend bool operator == (const TwoLevelFwdIter& left, const TwoLevelFwdIter& right) {
    return left.is_equal (right);
  }

  friend bool operator != (const TwoLevelFwdIter& left, const TwoLevelFwdIter& right) {
    return !left.is_equal (right);
  }


};

//! Two-Level bidirectional iterator
template <typename Outer, typename Inner, typename InnerBegFn, typename InnerEndFn>
class TwoLevelBiDirIter: public TwoLevelFwdIter<Outer, Inner, InnerBegFn, InnerEndFn> {

protected:
  typedef TwoLevelFwdIter<Outer, Inner, InnerBegFn, InnerEndFn> FwdBase;

protected:
  void prevOuter () {
    assert (!FwdBase::outerAtBegin ());
    assert (!FwdBase::outerEmpty ());

    TwoLevelIteratorImpl::safe_decrement (FwdBase::m_outer, FwdBase::m_beg_outer, FwdBase::m_end_outer);

    FwdBase::m_inner = FwdBase::innerEnd ();
  }


  void step_backward () {
    assert (!FwdBase::outerEmpty ());
    assert (!FwdBase::outerAtBegin ());

    // calling innerBegin when m_outer == m_end_outer is invalid
    // so call prevOuter first, and check for innerBegin afterwards

    if (FwdBase::outerAtEnd ()) {
      prevOuter ();
    }

    while (FwdBase::innerAtBegin ()) {
      assert (!FwdBase::outerAtBegin ());
      prevOuter ();
    }

    assert (FwdBase::innerAtBegin () ? FwdBase::outerAtBegin () : true);

    --FwdBase::m_inner;
  }

public:

  TwoLevelBiDirIter (): FwdBase () {}

  TwoLevelBiDirIter (
      Outer beg_outer, 
      Outer end_outer,
      InnerBegFn innerBegFn,
      InnerEndFn innerEndFn)
    : 
      FwdBase (beg_outer, end_outer, innerBegFn, innerEndFn)
  {}


  TwoLevelBiDirIter& operator -- () {
    step_backward ();
    return *this;
  }

  TwoLevelBiDirIter operator -- (int) {
    TwoLevelBiDirIter tmp (*this);
    step_backward ();
    return tmp;
  }
};


//! Two-Level random access iterator
template <typename Outer, typename Inner, typename InnerBegFn, typename InnerEndFn>
class TwoLevelRandIter: public TwoLevelBiDirIter<Outer, Inner, InnerBegFn, InnerEndFn> {

protected:
  typedef TwoLevelBiDirIter<Outer, Inner, InnerBegFn, InnerEndFn> BiDirBase;

  typedef typename BiDirBase::Traits::difference_type Diff_ty;

  void jump_forward (const Diff_ty d) {
    assert (!BiDirBase::outerEmpty ());

    if (d < 0) {
      jump_backward (-d);

    } else {
      Diff_ty rem (d);

      while (rem > 0) {
        assert (!BiDirBase::outerAtEnd ());

        Diff_ty avail = std::distance (BiDirBase::m_inner, BiDirBase::innerEnd ());
        assert (avail >= 0);

        if (rem > avail) {
          rem -= avail;
          assert (!BiDirBase::outerAtEnd ());
          BiDirBase::nextOuter ();

        } else {
          BiDirBase::m_inner += rem;
          rem = 0;
        }

        BiDirBase::seekValidBegin ();
      }
    }
  }

  void jump_backward (const Diff_ty d) {
    assert (!BiDirBase::outerEmpty ());

    if (d < 0) {
      jump_forward (-d);

    } else {


      Diff_ty rem (d);

      if ((rem > 0) && BiDirBase::outerAtEnd ()) {
        BiDirBase::prevOuter ();
        
      }

      while (rem > 0) {
        Diff_ty avail = std::distance (BiDirBase::innerBegin (), BiDirBase::m_inner);
        assert (avail >= 0);

        if (rem > avail) {
          rem -= avail;
          assert (!BiDirBase::outerAtBegin ());
          BiDirBase::prevOuter ();
          
        } else {

          BiDirBase::m_inner -= rem;
          rem = 0;
          break;
        }
      }
    }
  }

  Diff_ty compute_dist (const TwoLevelRandIter& that) const {

    if (std::distance (this->m_outer, that.m_outer) < 0) { // this->m_outer > that.m_outer
      return -(that.compute_dist (*this));

    } else if (this->m_outer == that.m_outer) {
      if (!BiDirBase::outerAtEnd ()) { 
        return std::distance (this->m_inner, that.m_inner);

      } else { 
        return 0;
      }

    } else { 

      assert (std::distance (this->m_outer, that.m_outer) > 0); // this->m_outer < that.m_outer;
      assert (!BiDirBase::outerAtEnd ());

      TwoLevelRandIter tmp (*this);

      Diff_ty d = tmp.m_inner - tmp.m_inner; // 0

      while (tmp.m_outer != that.m_outer) {
        d += std::distance (tmp.m_inner, tmp.innerEnd ());
        tmp.nextOuter ();
      }

      assert (tmp.m_outer == that.m_outer);

      if (tmp.m_outer != tmp.m_end_outer) {
        d += std::distance (tmp.m_inner, that.m_inner);
      }

      assert (d >= 0);

      return d;
    }
  }


public:

  TwoLevelRandIter (): BiDirBase () {}

  TwoLevelRandIter (
      Outer beg_outer, 
      Outer end_outer,
      InnerBegFn innerBegFn,
      InnerEndFn innerEndFn)
    : BiDirBase (beg_outer, end_outer, innerBegFn, innerEndFn) {}

  TwoLevelRandIter& operator += (Diff_ty d) {
    jump_forward (d);
    return *this;
  }

  TwoLevelRandIter& operator -= (Diff_ty d) {
    jump_backward (d);
    return *this;
  }

  friend TwoLevelRandIter operator + (const TwoLevelRandIter& it, Diff_ty d) {
    TwoLevelRandIter tmp (it);
    tmp += d;
    return tmp;
  }

  friend TwoLevelRandIter operator + (Diff_ty d, const TwoLevelRandIter& it) {
    return (it + d);
  }

  friend TwoLevelRandIter operator - (const TwoLevelRandIter& it, Diff_ty d) {
    TwoLevelRandIter tmp (it);
    tmp -= d;
    return tmp;
  }


  friend Diff_ty operator - (const TwoLevelRandIter& left, const TwoLevelRandIter& right) {

    return right.compute_dist (left);
  }

  typename BiDirBase::Traits::reference operator [] (Diff_ty d) const {
    return *((*this) + d);
  }

  friend bool operator < (const TwoLevelRandIter& left, const TwoLevelRandIter& right) {
    return ((left.m_outer == right.m_outer) ? (left.m_inner < right.m_inner) : (left.m_outer < right.m_outer));
  }

  friend bool operator <= (const TwoLevelRandIter& left, const TwoLevelRandIter& right) {
    return (left < right) || (left == right);
  }

  friend bool operator > (const TwoLevelRandIter& left, const TwoLevelRandIter& right) {
    return !(left <= right);
  }

  friend bool operator >= (const TwoLevelRandIter& left, const TwoLevelRandIter& right) {
    return !(left < right);
  }
};

namespace TwoLevelIteratorImpl {

template <typename Outer, typename Inner, typename InnerBegFn, typename InnerEndFn, typename Cat>
struct ByCategory {};

template <typename Outer, typename Inner, typename InnerBegFn, typename InnerEndFn> 
struct ByCategory<Outer, Inner, InnerBegFn, InnerEndFn, std::forward_iterator_tag> {
  typedef TwoLevelFwdIter<Outer, Inner, InnerBegFn, InnerEndFn> type;
};

template <typename Outer, typename Inner, typename InnerBegFn, typename InnerEndFn> 
struct ByCategory<Outer, Inner, InnerBegFn, InnerEndFn, std::bidirectional_iterator_tag> {
  typedef TwoLevelBiDirIter<Outer, Inner, InnerBegFn, InnerEndFn> type;
};

template <typename Outer, typename Inner, typename InnerBegFn, typename InnerEndFn> 
struct ByCategory<Outer, Inner, InnerBegFn, InnerEndFn, std::random_access_iterator_tag> {
  typedef TwoLevelRandIter<Outer, Inner, InnerBegFn, InnerEndFn> type;
};

// template <typename Outer, typename Inner>
// struct IsRvrsIter {
// 
  // template <typename O, typename I>
  // struct IsRev {
    // static const bool VAL = false;
  // };
// 
  // template <typename O>
  // struct IsRev<O, typename O::value_type::reverse_iterator> {
    // static const bool VAL = true;
  // };
// 
  // template <typename O, typename I>
  // struct IsConstRev {
    // static const bool VAL = false;
  // };
// 
  // template <typename O>
  // struct IsConstRev<O, typename O::value_type::const_reverse_iterator> {
    // static const bool VAL = true;
  // };
// 
// 
  // static const bool VAL = 
    // IsRev<Outer, Inner>::VAL || IsConstRev<Outer, Inner>::VAL;
// };

} // end namespace impl

//! Type function to select appropriate two-level iterator
template <typename Outer, typename Inner, typename InnerBegFn, typename InnerEndFn>
struct ChooseTwoLevelIterator {
private:
  // typedef typename std::iterator_traits<Outer>::iterator_category CatOuter;
  typedef typename std::iterator_traits<Inner>::iterator_category CatInner;

public:
  typedef typename TwoLevelIteratorImpl::ByCategory<Outer, Inner, InnerBegFn, InnerEndFn, CatInner>::type type;
};

//! Creates two level iterator
template <typename Outer, typename InnerBegFn, typename InnerEndFn>
typename ChooseTwoLevelIterator<Outer, typename InnerBegFn::result_type, InnerBegFn, InnerEndFn>::type
make_two_level_begin (Outer beg, Outer end, InnerBegFn innerBegFn, InnerEndFn innerEndFn) {
  const bool V = std::is_same<typename InnerBegFn::result_type, typename InnerEndFn::result_type>::value;
  assert (V);

  typedef typename InnerBegFn::result_type Inner;
  typedef typename ChooseTwoLevelIterator<Outer, Inner, InnerBegFn, InnerEndFn>::type Ret_ty;

  return Ret_ty (beg, end, innerBegFn, innerEndFn);
}

//! Creates two level iterator
template <typename Outer, typename InnerBegFn, typename InnerEndFn>
typename ChooseTwoLevelIterator<Outer, typename InnerBegFn::result_type, InnerBegFn, InnerEndFn>::type
make_two_level_end (Outer beg, Outer end, InnerBegFn innerBegFn, InnerEndFn innerEndFn) {
  const bool V = std::is_same<typename InnerBegFn::result_type, typename InnerEndFn::result_type>::value;
  assert (V);

  typedef typename InnerBegFn::result_type Inner;
  typedef typename ChooseTwoLevelIterator<Outer, Inner, InnerBegFn, InnerEndFn>::type Ret_ty;

  return Ret_ty (end, end, innerBegFn, innerEndFn);
}

namespace TwoLevelIteratorImpl {
  template <typename C>
  struct GetBegin: public std::unary_function<C&, typename C::iterator> {
    inline typename C::iterator operator () (C& c) const {
      return c.begin ();
    }
  };

  template <typename C>
  struct GetEnd: public std::unary_function<C&, typename C::iterator> {
    inline typename C::iterator operator () (C& c) const {
      return c.end ();
    }
  };

  // TODO: update to c++11 names
  template <typename C>
  struct GetCbegin: public std::unary_function<const C&, typename C::const_iterator> {
    inline typename C::const_iterator operator () (const C& c) const {
      return c.begin ();
    }
  };

  template <typename C>
  struct GetCend: public std::unary_function<const C&, typename C::const_iterator> {
    inline typename C::const_iterator operator () (const C& c) const {
      return c.end ();
    }
  };

  template <typename C>
  struct GetRbegin: public std::unary_function<C&, typename C::reverse_iterator> {
    inline typename C::reverse_iterator operator () (C& c) const {
      return c.rbegin ();
    }
  };

  template <typename C>
  struct GetRend: public std::unary_function<C&, typename C::reverse_iterator> {
    inline typename C::reverse_iterator operator () (C& c) const {
      return c.rend ();
    }
  };

  // TODO: update to c++11 names
  template <typename C>
  struct GetCRbegin: public std::unary_function<const C&, typename C::const_reverse_iterator> {
    inline typename C::const_reverse_iterator operator () (const C& c) const {
      return c.rbegin ();
    }
  };

  template <typename C>
  struct GetCRend: public std::unary_function<const C&, typename C::const_reverse_iterator> {
    inline typename C::const_reverse_iterator operator () (const C& c) const {
      return c.rend ();
    }
  };

  enum StlIterKind { NORMAL, CONST, REVERSE, CONST_REVERSE };

  template <typename C, typename I> struct IsConstIter 
  { static const bool value = false; };

  template <typename C> struct IsConstIter<C, typename C::const_iterator> 
  { static const bool value = true; };

  template <typename C, typename I> struct IsRvrsIter 
  { static const bool value = false; };

  template <typename C> struct IsRvrsIter<C, typename C::reverse_iterator> 
  { static const bool value = true; };

  template <typename C, typename I> struct IsRvrsConstIter 
  { static const bool value = false; };

  template <typename C> struct IsRvrsConstIter<C, typename C::const_reverse_iterator> 
  { static const bool value = true; };

  template <typename C, typename I>
  struct GetStlIterKind {
    static const bool isRvrs = IsRvrsIter<C, I>::value || IsRvrsConstIter<C, I>::value;
    static const bool isConst = IsConstIter<C, I>::value || IsRvrsConstIter<C, I>::value;

    static const StlIterKind value = 
      isRvrs ? (isConst ? CONST_REVERSE: REVERSE)
        : (isConst ? CONST : NORMAL);
  };

  template <typename C, typename I, enum StlIterKind> 
  struct ChooseStlIter { 
    typedef void Inner; 
  };

  template <typename C, typename I> 
  struct ChooseStlIter<C, I, NORMAL> { 

    typedef typename C::iterator Inner;
    typedef GetBegin<C> InnerBegFn;
    typedef GetEnd<C> InnerEndFn;

  };

  template <typename C, typename I> 
  struct ChooseStlIter<C, I, CONST> { 

    typedef typename C::const_iterator Inner; 
    typedef GetCbegin<C> InnerBegFn;
    typedef GetCend<C> InnerEndFn;
  };

  template <typename C, typename I> 
  struct ChooseStlIter<C, I, REVERSE> { 

    typedef typename C::reverse_iterator Inner; 
    typedef GetRbegin<C> InnerBegFn;
    typedef GetRend<C> InnerEndFn;
  };

  template <typename C, typename I> 
  struct ChooseStlIter<C, I, CONST_REVERSE> { 

    typedef typename C::const_reverse_iterator Inner; 
    typedef GetCRbegin<C> InnerBegFn;
    typedef GetCRend<C> InnerEndFn;
  };

  template <typename Outer, typename Inner>
  struct ChooseStlTwoLevelIterImpl {

    typedef typename std::iterator_traits<Outer>::value_type C;
    static const TwoLevelIteratorImpl::StlIterKind KIND = TwoLevelIteratorImpl::GetStlIterKind<C, Inner>::value;
    typedef TwoLevelIteratorImpl::ChooseStlIter<C, Inner, KIND> CStl;
    typedef typename CStl::InnerBegFn InnerBegFn;
    typedef typename CStl::InnerEndFn InnerEndFn;
    typedef typename ChooseTwoLevelIterator<Outer, Inner, InnerBegFn, InnerEndFn>::type type;

    static type make (Outer beg, Outer end) {
      return type (beg, end, InnerBegFn (), InnerEndFn ());
    }
  };

  template <typename Outer> struct StlInnerIsIterator
  : public ChooseStlTwoLevelIterImpl<Outer, typename std::iterator_traits<Outer>::value_type::iterator> {};

  template <typename Outer> struct StlInnerIsConstIterator
  : public ChooseStlTwoLevelIterImpl<Outer, typename std::iterator_traits<Outer>::value_type::const_iterator> {};

  template <typename Outer> struct StlInnerIsRvrsIterator
  : public ChooseStlTwoLevelIterImpl<Outer, typename std::iterator_traits<Outer>::value_type::reverse_iterator> {};

  template <typename Outer> struct StlInnerIsConstRvrsIterator
  : public ChooseStlTwoLevelIterImpl<Outer, typename std::iterator_traits<Outer>::value_type::const_reverse_iterator> {};

} // end namespace impl

//! Type function to select appropriate two-level iterator
template <typename Outer, typename Inner>
struct ChooseStlTwoLevelIterator {
  typedef typename TwoLevelIteratorImpl::ChooseStlTwoLevelIterImpl<Outer, Inner>::type type;
};

template <typename Outer>
typename TwoLevelIteratorImpl::StlInnerIsIterator<Outer>::type 
stl_two_level_begin (Outer beg, Outer end) {
  return TwoLevelIteratorImpl::StlInnerIsIterator<Outer>::make (beg, end);
}

template <typename Outer>
typename TwoLevelIteratorImpl::StlInnerIsIterator<Outer>::type
stl_two_level_end (Outer beg, Outer end) {
  return TwoLevelIteratorImpl::StlInnerIsIterator<Outer>::make (end, end);
}

template <typename Outer>
typename TwoLevelIteratorImpl::StlInnerIsConstIterator<Outer>::type
stl_two_level_cbegin (Outer beg, Outer end) {
  return TwoLevelIteratorImpl::StlInnerIsConstIterator<Outer>::make (beg, end);
}

template <typename Outer>
typename TwoLevelIteratorImpl::StlInnerIsConstIterator<Outer>::type 
stl_two_level_cend (Outer beg, Outer end) {
  return TwoLevelIteratorImpl::StlInnerIsConstIterator<Outer>::make (end, end);
}

template <typename Outer>
typename TwoLevelIteratorImpl::StlInnerIsRvrsIterator<Outer>::type
stl_two_level_rbegin (Outer beg, Outer end) {
  return TwoLevelIteratorImpl::StlInnerIsRvrsIterator<Outer>::make (beg, end);
}

template <typename Outer>
typename TwoLevelIteratorImpl::StlInnerIsRvrsIterator<Outer>::type
stl_two_level_rend (Outer beg, Outer end) {
  return TwoLevelIteratorImpl::StlInnerIsRvrsIterator<Outer>::make (end, end);
}

template <typename Outer>
typename TwoLevelIteratorImpl::StlInnerIsConstRvrsIterator<Outer>::type
stl_two_level_crbegin (Outer beg, Outer end) {
  return TwoLevelIteratorImpl::StlInnerIsConstRvrsIterator<Outer>::make (beg, end);
}

template <typename Outer>
typename TwoLevelIteratorImpl::StlInnerIsConstRvrsIterator<Outer>::type
stl_two_level_crend (Outer beg, Outer end) {
  return TwoLevelIteratorImpl::StlInnerIsConstRvrsIterator<Outer>::make (end, end);
}


} // end namespace Galois

#endif // GALOIS_TWO_LEVEL_ITER_H
