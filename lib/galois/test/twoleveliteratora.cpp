#include "Galois/config.h"
#include "Galois/TwoLevelIteratorA.h"
#include "Galois/Runtime/ll/gio.h"

#include <algorithm>
#include <boost/iterator/counting_iterator.hpp>
#include GALOIS_CXX11_STD_HEADER(vector)
#include <list>
#include <iostream>

const int N = 1024 * 4;

template<class D, class I>
struct GetBegin {
  typename I::iterator operator()(typename D::reference x) const { return x.begin(); }
  typename I::const_iterator operator()(typename D::const_reference x) const { return x.begin(); }
};

template<class D, class I>
struct GetEnd {
  typename I::iterator operator()(typename D::reference x) const { return x.end(); }
  typename I::const_iterator operator()(typename D::const_reference x) const { return x.end(); }
};

template<bool NonEmpty, class Tag, class D, class I>
void check_forward() {
  D data;

  for (int i = 0; i < N; ++i) {
#ifdef GALOIS_CXX11_VECTOR_HAS_NO_EMPLACE
    if (NonEmpty) {
      data.push_back(typename D::value_type());
      data.back().push_back(i);
    } else {
      data.push_back(typename D::value_type());
      data.push_back(typename D::value_type());
      data.back().push_back(i);
      data.push_back(typename D::value_type());
    }
#else
    if (NonEmpty) {
      data.emplace_back();
      data.back().push_back(i);
    } else {
      data.emplace_back();
      data.emplace_back();
      data.back().push_back(i);
      data.emplace_back();
    }
#endif
  }

#if __cplusplus >= 201103L
  auto r = Galois::make_two_level_iterator<Tag>(data.begin(), data.end());
#else
  auto r = Galois::make_two_level_iterator<
    Tag,
    typename D::iterator,
    typename I::iterator,
    GetBegin<D, I>,
    GetEnd<D, I>
    >(data.begin(), data.end());
#endif
  if (!std::equal(r.first, r.second, boost::make_counting_iterator<int>(0))) {
    GALOIS_DIE("failed case: forward ", (NonEmpty ? "non-empty" : "empty"),  " inner range");
  } else if (std::distance(r.first, r.second) != N) {
    GALOIS_DIE("failed case: forward ", (NonEmpty ? "non-empty" : "empty"),  " inner range: ",
        std::distance(r.first, r.second), " != ", N);
  }
}

template<bool NonEmpty, class Tag, class D, class I>
void check_backward() {
  D data;

  for (int i = N-1; i >= 0; --i) {
#ifdef GALOIS_CXX11_VECTOR_HAS_NO_EMPLACE
    if (NonEmpty) {
      data.push_back(typename D::value_type());
      data.back().push_back(i);
    } else {
      data.push_back(typename D::value_type());
      data.push_back(typename D::value_type());
      data.back().push_back(i);
      data.push_back(typename D::value_type());
    }
#else
    if (NonEmpty) {
      data.emplace_back();
      data.back().push_back(i);
    } else {
      data.emplace_back();
      data.emplace_back();
      data.back().push_back(i);
      data.emplace_back();
    }
#endif
  }

#if __cplusplus >= 201103L
  auto r = Galois::make_two_level_iterator<Tag>(data.begin(), data.end());
#else
  auto r = Galois::make_two_level_iterator<
    Tag,
    typename D::iterator,
    typename I::iterator,
    GetBegin<D, I>,
    GetEnd<D, I>
    >(data.begin(), data.end());
#endif
  auto c = boost::make_counting_iterator<int>(0);
  if (std::distance(r.first, r.second) != N) {
    GALOIS_DIE("failed case: backward ", (NonEmpty ? "non-empty" : "empty"), " inner range: ",
        std::distance(r.first, r.second), " != ", N);
  } else if (r.first == r.second) {
    return;
  }

  --r.second;
  while (true) {
    if (*r.second != *c) {
      GALOIS_DIE("failed case: backward ", (NonEmpty ? "non-empty" : "empty"), " inner range: ", 
          *r.second, " != ", *c);
    }
    if (r.first == r.second)
      break;
    --r.second;
    ++c;
  }
}

template<bool NonEmpty, class Tag, class D, class I>
void check_strided() {
  D data;

  for (int i = 0; i < N; ++i) {
#ifdef GALOIS_CXX11_VECTOR_HAS_NO_EMPLACE
    if (NonEmpty) {
      data.push_back(typename D::value_type());
      data.back().push_back(i);
    } else {
      data.push_back(typename D::value_type());
      data.push_back(typename D::value_type());
      data.back().push_back(i);
      data.push_back(typename D::value_type());
    }
#else
    if (NonEmpty) {
      data.emplace_back();
      data.back().push_back(i);
    } else {
      data.emplace_back();
      data.emplace_back();
      data.back().push_back(i);
      data.emplace_back();
    }
#endif
  }

#if __cplusplus >= 201103L
  auto r = Galois::make_two_level_iterator<Tag>(data.begin(), data.end());
#else
  auto r = Galois::make_two_level_iterator<
    Tag,
    typename D::iterator,
    typename I::iterator,
    GetBegin<D, I>,
    GetEnd<D, I>
    >(data.begin(), data.end());
#endif
  auto c = boost::make_counting_iterator<int>(0);
  if (std::distance(r.first, r.second) != N) {
    GALOIS_DIE("failed case: strided ", (NonEmpty ? "non-empty" : "empty"), " inner range: ",
        std::distance(r.first, r.second), " != ", N);
  } else if (r.first == r.second) {
    return;
  }

  while (r.first != r.second) {
    if (*r.first != *c) {
      GALOIS_DIE("failed case: strided ", (NonEmpty ? "non-empty" : "empty"), " inner range: ", 
          *r.first, " != ", *c);
    }
    
    auto orig = r.first;

    int k = std::max((N - *c) / 2, 1);
    std::advance(r.first, k);
    if (std::distance(orig, r.first) != k) {
      GALOIS_DIE("failed case: strided ", (NonEmpty ? "non-empty" : "empty"), " inner range: ", 
          std::distance(orig, r.first), " != ", k);
    }
    for (int i = 0; i < k - 1; ++i)
      std::advance(r.first, -1);

    if (std::distance(orig, r.first) != 1) {
      GALOIS_DIE("failed case: strided ", (NonEmpty ? "non-empty" : "empty"), " inner range: ", 
          std::distance(orig, r.first), " != 1");
    }

    ++c;
  }
}

void check_forward_iteration() {
  check_forward<true, std::forward_iterator_tag, std::vector<std::vector<int>>, std::vector<int>>();
  check_forward<true, std::forward_iterator_tag, std::vector<std::list<int>>, std::list<int>>();
  check_forward<true, std::forward_iterator_tag, std::list<std::vector<int>>, std::vector<int>>();
  check_forward<true, std::forward_iterator_tag, std::list<std::list<int>>, std::list<int>>();

  check_forward<true, std::bidirectional_iterator_tag, std::vector<std::vector<int>>, std::vector<int>>();
  check_forward<true, std::bidirectional_iterator_tag, std::vector<std::list<int>>, std::list<int>>();
  check_forward<true, std::bidirectional_iterator_tag, std::list<std::vector<int>>, std::vector<int>>();
  check_forward<true, std::bidirectional_iterator_tag, std::list<std::list<int>>, std::list<int>>();

  check_forward<true, std::random_access_iterator_tag, std::vector<std::vector<int>>, std::vector<int>>();
  check_forward<true, std::random_access_iterator_tag, std::vector<std::list<int>>, std::list<int>>();
  check_forward<true, std::random_access_iterator_tag, std::list<std::vector<int>>, std::vector<int>>();
  check_forward<true, std::random_access_iterator_tag, std::list<std::list<int>>, std::list<int>>();

  check_forward<false, std::forward_iterator_tag, std::vector<std::vector<int>>, std::vector<int>>();
  check_forward<false, std::forward_iterator_tag, std::vector<std::list<int>>, std::list<int>>();
  check_forward<false, std::forward_iterator_tag, std::list<std::vector<int>>, std::vector<int>>();
  check_forward<false, std::forward_iterator_tag, std::list<std::list<int>>, std::list<int>>();

  check_forward<false, std::bidirectional_iterator_tag, std::vector<std::vector<int>>, std::vector<int>>();
  check_forward<false, std::bidirectional_iterator_tag, std::vector<std::list<int>>, std::list<int>>();
  check_forward<false, std::bidirectional_iterator_tag, std::list<std::vector<int>>, std::vector<int>>();
  check_forward<false, std::bidirectional_iterator_tag, std::list<std::list<int>>, std::list<int>>();

  check_forward<false, std::random_access_iterator_tag, std::vector<std::vector<int>>, std::vector<int>>();
  check_forward<false, std::random_access_iterator_tag, std::vector<std::list<int>>, std::list<int>>();
  check_forward<false, std::random_access_iterator_tag, std::list<std::vector<int>>, std::vector<int>>();
  check_forward<false, std::random_access_iterator_tag, std::list<std::list<int>>, std::list<int>>();
}

void check_backward_iteration() {
  check_backward<true, std::bidirectional_iterator_tag, std::vector<std::vector<int>>,std::vector<int>>();
  check_backward<true, std::bidirectional_iterator_tag, std::vector<std::list<int>>,std::list<int>>();
  check_backward<true, std::bidirectional_iterator_tag, std::list<std::vector<int>>,std::vector<int>>();
  check_backward<true, std::bidirectional_iterator_tag, std::list<std::list<int>>,std::list<int>>();

  check_backward<true, std::random_access_iterator_tag, std::vector<std::vector<int>>,std::vector<int>>();
  check_backward<true, std::random_access_iterator_tag, std::vector<std::list<int>>,std::list<int>>();
  check_backward<true, std::random_access_iterator_tag, std::list<std::vector<int>>,std::vector<int>>();
  check_backward<true, std::random_access_iterator_tag, std::list<std::list<int>>,std::list<int>>();

  check_backward<false, std::bidirectional_iterator_tag, std::vector<std::vector<int>>,std::vector<int>>();
  check_backward<false, std::bidirectional_iterator_tag, std::vector<std::list<int>>,std::list<int>>();
  check_backward<false, std::bidirectional_iterator_tag, std::list<std::vector<int>>,std::vector<int>>();
  check_backward<false, std::bidirectional_iterator_tag, std::list<std::list<int>>,std::list<int>>();

  check_backward<false, std::random_access_iterator_tag, std::vector<std::vector<int>>,std::vector<int>>();
  check_backward<false, std::random_access_iterator_tag, std::vector<std::list<int>>,std::list<int>>();
  check_backward<false, std::random_access_iterator_tag, std::list<std::vector<int>>,std::vector<int>>();
  check_backward<false, std::random_access_iterator_tag, std::list<std::list<int>>,std::list<int>>();
}

void check_strided_iteration() {
  check_strided<true, std::bidirectional_iterator_tag, std::vector<std::vector<int>>,std::vector<int>>();
  check_strided<true, std::bidirectional_iterator_tag, std::vector<std::list<int>>,std::list<int>>();
  check_strided<true, std::bidirectional_iterator_tag, std::list<std::vector<int>>,std::vector<int>>();
  check_strided<true, std::bidirectional_iterator_tag, std::list<std::list<int>>,std::list<int>>();

  check_strided<true, std::random_access_iterator_tag, std::vector<std::vector<int>>,std::vector<int>>();
  check_strided<true, std::random_access_iterator_tag, std::vector<std::list<int>>,std::list<int>>();
  check_strided<true, std::random_access_iterator_tag, std::list<std::vector<int>>,std::vector<int>>();
  check_strided<true, std::random_access_iterator_tag, std::list<std::list<int>>,std::list<int>>();

  check_strided<false, std::bidirectional_iterator_tag, std::vector<std::vector<int>>,std::vector<int>>();
  check_strided<false, std::bidirectional_iterator_tag, std::vector<std::list<int>>,std::list<int>>();
  check_strided<false, std::bidirectional_iterator_tag, std::list<std::vector<int>>,std::vector<int>>();
  check_strided<false, std::bidirectional_iterator_tag, std::list<std::list<int>>,std::list<int>>();

  check_strided<false, std::random_access_iterator_tag, std::vector<std::vector<int>>,std::vector<int>>();
  check_strided<false, std::random_access_iterator_tag, std::vector<std::list<int>>,std::list<int>>();
  check_strided<false, std::random_access_iterator_tag, std::list<std::vector<int>>,std::vector<int>>();
  check_strided<false, std::random_access_iterator_tag, std::list<std::list<int>>,std::list<int>>();
}

int main() {
  typedef std::vector<std::vector<int>> NestedVector;

  // Static checks
  NestedVector data;
  const NestedVector& d(data);
#if __cplusplus >= 201103L
  auto r = Galois::make_two_level_iterator(d.begin(), d.end());
#else
  auto r = Galois::make_two_level_iterator<
    std::forward_iterator_tag,
    NestedVector::const_iterator,
    std::vector<int>::const_iterator,
    GetBegin<NestedVector, std::vector<int> >,
    GetEnd<NestedVector, std::vector<int> > >(d.begin(), d.end());
#endif
  static_assert(std::is_same<decltype(*r.first), const int&>::value, "failed case: preserve constness");

  // Runtime checks
  check_forward_iteration();
  check_backward_iteration();
  check_strided_iteration();

  return 0;
}
