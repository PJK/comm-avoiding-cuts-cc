//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <boost/config.hpp>
#include <iostream>
#include <vector>
#include <stdint.h>
#include <algorithm>
#include <numeric>
#include <utility>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/connected_components.hpp>
#include "utils.hpp"
#include "GraphInputIterator.hpp"


using namespace std;

int main(int argc, char* argv[])
{
	using namespace boost;
	{
		if ((argc != 2) && (argc != 3)) {
			std::cout << "Usage: connected_components INPUT_FILE [ITERATONS]" << std::endl;
			return 1;
		}

		GraphInputIterator input(argv[1]);
		int iterations = 1;

		if (argc == 3) {
			iterations = std::stoi(argv[2]);
		}

		typedef adjacency_list <vecS, vecS, undirectedS> Graph;

		Graph g(input.vertexCount());

		for (auto edge : input) {
			boost::add_edge(
					boost::vertex(edge.from, g),
					boost::vertex(edge.to, g),
					g
			);
		}


		std::cout << std::fixed;

		for (int i = 0; i < iterations; i++) {
			double time;
			int num;

			CacheUtils::trashCache();

			TimeUtils::measure<void>([&]() {
				PAPI_START();
				std::vector<int> component(num_vertices(g));
				num = connected_components(g, &component[0]);
				PAPI_STOP(0, 0);
			}, time);

			std::cout << argv[1] << ","
					  << ","
					  << 1 << ","
					  << input.vertexCount() << ","
					  << input.edgeCount() << ","
					  << time <<  ","
					  << ","
					  << "bgl" << ","
					  << num << std::endl;
		}

	}
	return 0;
}
