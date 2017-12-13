#include "Galois/Galois.h"
#include "Galois/ParallelSTL/ParallelSTL.h"
#include "Galois/Timer.h"

#include <iostream>
#include <cstdlib>
#include <numeric>

int RandomNumber () { return (rand()%1000000); }
bool IsOdd (int i) { return ((i%2)==1); }

struct IsOddS {
  bool operator() (int i) { return ((i%2)==1); }
};

int do_sort() {

  unsigned M = Galois::Runtime::LL::getMaxThreads();
  std::cout << "sort:\n";

  while (M) {
    
    Galois::setActiveThreads(M); //Galois::Runtime::LL::getMaxThreads());
    std::cout << "Using " << M << " threads\n";
    
    std::vector<unsigned> V(1024*1024*16);
    std::generate (V.begin(), V.end(), RandomNumber);
    std::vector<unsigned> C = V;

    Galois::Timer t;
    t.start();
    Galois::ParallelSTL::sort(V.begin(), V.end());
    t.stop();
    
    Galois::Timer t2;
    t2.start();
    std::sort(C.begin(), C.end());
    t2.stop();

    bool eq = std::equal(C.begin(), C.end(), V.begin());

    std::cout << "Galois: " << t.get()
	      << " STL: " << t2.get()
	      << " Equal: " << eq << "\n";
    
    if (!eq) {
      std::vector<unsigned> R = V;
      std::sort(R.begin(), R.end());
      if (!std::equal(C.begin(), C.end(), R.begin()))
	std::cout << "Cannot be made equal, sort mutated array\n";
      for (size_t x = 0; x < V.size() ; ++x) {
	std::cout << x << "\t" << V[x] << "\t" << C[x];
	if (V[x] != C[x]) std::cout << "\tDiff";
	if (V[x] < C[x]) std::cout << "\tLT";
	if (V[x] > C[x]) std::cout << "\tGT";
	std::cout << "\n";
      }
      return 1;
    }

    M >>= 1;
  }

  return 0;
}

int do_count_if() {

  unsigned M = Galois::Runtime::LL::getMaxThreads();
  std::cout << "count_if:\n";

  while (M) {
    
    Galois::setActiveThreads(M); //Galois::Runtime::LL::getMaxThreads());
    std::cout << "Using " << M << " threads\n";
    
    std::vector<unsigned> V(1024*1024*16);
    std::generate (V.begin(), V.end(), RandomNumber);

    unsigned x1,x2;

    Galois::Timer t;
    t.start();
    x1 = Galois::ParallelSTL::count_if(V.begin(), V.end(), IsOddS());
    t.stop();
    
    Galois::Timer t2;
    t2.start();
    x2 = std::count_if(V.begin(), V.end(), IsOddS());
    t2.stop();

    std::cout << "Galois: " << t.get() 
	      << " STL: " << t2.get() 
	      << " Equal: " << (x1 == x2) << "\n";
    M >>= 1;
  }
  
  return 0;
}

template<typename T>
struct mymax : std:: binary_function<T,T,T> {
  T operator()(const T& x, const T& y) const {
    return std::max(x,y);
  }
};


int do_accumulate() {

  unsigned M = Galois::Runtime::LL::getMaxThreads();
  std::cout << "accumulate:\n";

  while (M) {
    
    Galois::setActiveThreads(M); //Galois::Runtime::LL::getMaxThreads());
    std::cout << "Using " << M << " threads\n";
    
    std::vector<unsigned> V(1024*1024*16);
    std::generate (V.begin(), V.end(), RandomNumber);

    unsigned x1,x2;

    Galois::Timer t;
    t.start();
    x1 = Galois::ParallelSTL::accumulate(V.begin(), V.end(), 0U, mymax<unsigned>());
    t.stop();
    
    Galois::Timer t2;
    t2.start();
    x2 = std::accumulate(V.begin(), V.end(), 0U, mymax<unsigned>());
    t2.stop();

    std::cout << "Galois: " << t.get() 
	      << " STL: " << t2.get() 
	      << " Equal: " << (x1 == x2) << "\n";
    if (x1 != x2)
      std::cout << x1 << " " << x2 << "\n";
    M >>= 1;
  }
  
  return 0;
}

int main() {
  int ret = 0;
  //  ret |= do_sort();
  //  ret |= do_count_if();
  ret |= do_accumulate();
  return ret;
}
