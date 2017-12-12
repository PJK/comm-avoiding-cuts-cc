#include "input/GraphInputIterator.hpp"
#include "sum_tree.hpp"
#include "sparse_graph.hpp"
#include "stack_allocator.h"
#include "co_mincut.h"
#include "utils.hpp"

int main(int argc, char* argv[])
{
	if (argc != 3) {
		std::cout << "Usage: karger_stein INPUT_FILE SEED" << std::endl;
		return 1;
	}

	GraphInputIterator input(argv[1]);

	uint32_t seed = { (uint32_t) std::stoi(argv[2]) };

	stack_allocator allocator(sizeof(edge<unsigned long>) * input.edgeCount());
	array<edge<unsigned long>> ks_edges = allocator.allocate<edge<unsigned long>>(input.edgeCount());

	GraphInputIterator::Iterator input_edges = input.begin();
	for (unsigned i { 0 }; i < input.edgeCount(); i++) {
		edge<unsigned long> e;
		e.set_vertices(input_edges->from, input_edges->to);
		e.weight = input_edges->weight;
		ks_edges[i] = e;
		++input_edges;
	}

	sparse_graph<unsigned long, edge<unsigned long>> ks_graph(input.vertexCount(), ks_edges);

	double time;
	PAPI_START();
	unsigned cut_value = TimeUtils::measure<unsigned>([&]() { return comincut::minimum_cut(&ks_graph, 0.90, seed); }, time);
	PAPI_STOP(0, 0);

	std::cout << argv[1] << ","
			  << seed << ","
			  << 1 << ","
			  << input.vertexCount() << ","
			  << input.edgeCount() << ","
			  << time << ","
			  << "?" << ","
			  << "co-karger-stein,"
			  << cut_value << std::endl;
}
