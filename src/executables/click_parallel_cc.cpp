#include <mpi.h>
#include <iostream>
#include "UnweightedIteratedSparseSampling.hpp"
#include "CLICKIteratedSampling.hpp"
#include "utils.hpp"

int main(int argc, char* argv[])
{
	if ((argc != 4) && (argc != 5)) {
		std::cout << "Usage: connected_components N AVGD SEED [ITERATONS]" << std::endl;
		return 1;
	}

	unsigned n = std::stoul(argv[1]), d = std::stoul(argv[2]);

	uint32_t seed = { (uint32_t) std::stoi(argv[3]) };
	int iterations = 1;

	if (argc == 5) {
		iterations = std::stoi(argv[4]);
	}

	MPI_Init(&argc, &argv);


	int rank, p;

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &p);

	// Use CLICK sampler to generate the slice for the unweighted implementaiton
	CLICKIteratedSampling click(MPI_COMM_WORLD, 0, p, seed + rank, n, n * d);
	click.loadSlice();

	std::vector<UnweightedGraph::Edge> edges;
	UnweightedIteratedSparseSampling sampler(MPI_COMM_WORLD, 0, p, seed + rank, 1, n, n * d);

	for (auto edge : click.edgeSlice()) {
		edges.push_back(edge.dropWeight());
	}

	for (int i = 0; i < iterations; i++) {
		UnweightedIteratedSparseSampling iteration_sampler(sampler);
		MPI::total = 0;

		std::vector<unsigned> components;

		double time;
		int number_of_components = TimeUtils::measure<int>([&]() {
			return iteration_sampler.connectedComponents(components);
		}, time);

		double global_mpi;
		MPI_Reduce(&MPI::total, &global_mpi, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

		if (sampler.master()) {
			std::cout << std::fixed;
			std::cout << argv[1] << ","
					  << seed << ","
					  << p << ","
					  << n << ","
					  << n * d << ","
					  << time << ","
					  << global_mpi << ","
					  << "cc" << ","
					  << number_of_components << std::endl;
		}
		seed++;
	}

	MPI_Finalize();
}
