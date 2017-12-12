#include <mpi.h>
#include <iostream>
#include "UnweightedIteratedSparseSampling.hpp"
#include "utils.hpp"

int main(int argc, char* argv[])
{
	if ((argc != 3) && (argc != 4)) {
		std::cout << "Usage: connected_components INPUT_FILE SEED [ITERATONS]" << std::endl;
		return 1;
	}

	uint32_t seed = { (uint32_t) std::stoi(argv[2]) };
	int iterations = 1;

	if (argc == 4) {
		iterations = std::stoi(argv[3]);
	}

	MPI_Init(&argc, &argv);

	GraphInputIterator input(argv[1]);
	int rank, p;

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &p);

	UnweightedIteratedSparseSampling sampler(MPI_COMM_WORLD, 0, p, seed + rank, 1, input.vertexCount(), input.edgeCount());
	sampler.loadSlice(input);

	for (int i = 0; i < iterations; i++) {
		UnweightedIteratedSparseSampling iteration_sampler(sampler);

		std::vector<unsigned> components;
		CacheUtils::trashCache(std::max(1, 20 / p));

		MPI_Barrier(MPI_COMM_WORLD);
		MPI::total = 0;
		double time;
		PAPI_START();

		int number_of_components = TimeUtils::measure<int>([&]() {
			return iteration_sampler.connectedComponents(components);
		}, time);
		PAPI_STOP(rank, MPI::total);

		double global_mpi;
		MPI_Reduce(&MPI::total, &global_mpi, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

		if (sampler.master()) {
			std::cout << std::fixed;
			std::cout << argv[1] << ","
					  << seed << ","
					  << p << ","
					  << input.vertexCount() << ","
					  << input.edgeCount() << ","
					  << time << ","
					  << MPI::total << ","
					  << "cc" << ","
					  << number_of_components << std::endl;
		}
		seed++;
	}

	MPI_Finalize();
}
