#include "../sorting/SamplingSorter.hpp"
#include <vector>
#include <random>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <functional>
#include <limits>

using namespace std;

int main(int argc, char* argv[]) {
	MPI_Init(&argc, &argv);
	int rank, p;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &p);

	// http://stackoverflow.com/questions/21516575/fill-a-vector-with-random-numbers-c
	// First create an instance of an engine.
	random_device rnd_device;
	// Specify the engine and distribution.
	mt19937 mersenne_engine(rnd_device());
	uniform_int_distribution<int> dist(0, numeric_limits<int>::max());

	auto gen = std::bind(dist, mersenne_engine);
	vector<int> data(10000);
	generate(begin(data), end(data), gen);

	SamplingSorter<int> sorter(MPI_COMM_WORLD, move(data), rank);
	vector<int> result = sorter.sort();

	// Verify sortedness across slices
	vector<int> my_boundaries;
	my_boundaries.push_back(result.front());
	my_boundaries.push_back(result.back());

	if (rank == 0) {
		vector<int> boundaries(p * 2);
		MPI_Gather(
				my_boundaries.data(),
				2,
				MPI_INT,
				boundaries.data(),
				2,
				MPI_INT,
				0,
				MPI_COMM_WORLD
		);

		if (!std::is_sorted(boundaries.begin(), boundaries.end())) {
			throw std::runtime_error("Some slices are not sorted!");
		}
	} else {
		MPI_Gather(
				my_boundaries.data(),
				2,
				MPI_INT,
				nullptr,
				2,
				MPI_INT,
				0,
				MPI_COMM_WORLD
		);
	}

	MPI_Finalize();
}
