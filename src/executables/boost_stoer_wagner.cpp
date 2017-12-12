#include "input/GraphInputIterator.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/stoer_wagner_min_cut.hpp>
#include "../utils.hpp"

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS,
		boost::no_property, boost::property<boost::edge_weight_t, AdjacencyListGraph::Weight>> undirected_graph;
typedef boost::property_map<undirected_graph, boost::edge_weight_t>::type weight_map_type;

int main(int argc, char* argv[])
{
	if (argc != 2) {
		std::cout << "Usage: boost_stoer_wagner INPUT_FILE" << std::endl;
		return 1;
	}

	GraphInputIterator input(argv[1]);
	undirected_graph g;

	for (auto const edge : input)
		boost::add_edge(edge.from, edge.to, edge.weight, g);

	PAPI_START();

	double time;
	AdjacencyListGraph::Weight cut = TimeUtils::measure<AdjacencyListGraph::Weight>([&]() {
		return boost::stoer_wagner_min_cut(g, get(boost::edge_weight, g));
	}, time);

	PAPI_STOP(0, 0);

	std::cout << argv[1] << ","
			  << 0 << ","
			  << 1 << ","
			  << input.vertexCount() << ","
			  << input.edgeCount() << ","
			  << time << ","
			  << 1 << ","
			  << "stoer-wagner,"
			  << cut << std::endl;
}
