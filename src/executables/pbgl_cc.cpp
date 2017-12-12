#include <boost/graph/use_mpi.hpp>
#include <boost/config.hpp>
#include <boost/throw_exception.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/graph/distributed/adjacency_list.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/graph/distributed/connected_components.hpp>
#include <boost/graph/random.hpp>
#include <boost/property_map/parallel/distributed_property_map.hpp>
#include <boost/graph/distributed/mpi_process_group.hpp>
#include <boost/graph/parallel/distribution.hpp>
#include <boost/graph/copy.hpp>

#include "input/GraphInputIterator.hpp"
#include "utils.hpp"

using boost::graph::distributed::mpi_process_group;
using namespace boost::graph;

typedef boost::adjacency_list<boost::vecS,
		boost::distributedS<mpi_process_group, boost::vecS>,
		boost::undirectedS
> Graph;

struct Edge {
	unsigned from, to;
};

int main(int argc, char* argv[]) {
	boost::mpi::environment env(argc, argv);

	GraphInputIterator input(argv[1]);
	int iterations = 1;

	if (argc == 3) {
		iterations = std::stoi(argv[2]);
	}

	mpi_process_group pg;
	boost::parallel::variant_distribution<mpi_process_group> distrib = boost::parallel::block(pg, input.vertexCount());

	Graph g(input.vertexCount(), pg, distrib);

	std::vector<Edge> edges;

	if (process_id(g.process_group()) == 0) {
		for (auto edge : edges) {
			boost::add_edge(
					boost::vertex(edge.from, g),
					boost::vertex(edge.to, g),
					g
			);
		}
	}

	boost::synchronize(g);

	for (int i = 0; i < iterations; i++) {
		double time;
		boost::synchronize(g);
		PAPI_START();

		TimeUtils::measure<void>([&]() {
			std::vector<int> local_components_vec(num_vertices(g));
			typedef boost::iterator_property_map<std::vector<int>::iterator, boost::property_map<Graph, boost::vertex_index_t>::type> ComponentMap;
			ComponentMap component(local_components_vec.begin(), get(boost::vertex_index, g));
			connected_components(g, component);
		}, time);
		PAPI_STOP(process_id(g.process_group()), 0);

		if (process_id(g.process_group()) == 0) {
			std::cout << std::fixed;
			std::cout << argv[1] << ","
					  << ","
					  << num_processes(g.process_group()) << ","
					  << input.vertexCount() << ","
					  << input.edgeCount() << ","
					  << time <<  ","
					  << ","
					  << "bgl" << ","
					  << std::endl;
		}
	}
}
