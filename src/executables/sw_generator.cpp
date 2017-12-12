#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/small_world_generator.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <ostream>

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS> Graph;
typedef boost::small_world_iterator<boost::minstd_rand, Graph> SWGen;

// The NetworkX is quadratic and constantly slow -- code from my thesis

/**
 * Writes out any BGL-iterable graph to .grp
 *
 * TODO this assumes bidirectedS -- check for traits?
 * TODO this is so ugly because BGL_FORALL_VERTICES cannot take a dependent type argument
 */
#define SERIALIZE(GraphType, g, out) \
	{ \
		out << "# SW graph" << std::endl; \
		out << size_t(boost::num_vertices(g)) << " " << size_t(boost::num_edges(g)) << std::endl; \
		BGL_FORALL_EDGES(e, g, GraphType) { \
			out << boost::source(e, g) << " " << boost::target(e, g) << " " << 100 << std::endl; \
		} \
	}


// Usage N avg_d
int main(int argc, char* argv[]) {
	unsigned long n = std::stoul(argv[1]);
	unsigned long d = std::stoul(argv[2]);

	boost::minstd_rand gen;
	Graph g(SWGen(gen, n, d * 2, 0.3), SWGen(), n);

	SERIALIZE(Graph, g, std::cout);

	return 0;
}
