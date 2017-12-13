#include <iostream>
#include <vector>
#include <mpi.h>
#include <random>
#include "parmetis.h"
#include "utils.hpp"
#include "GraphInputIterator.hpp"

struct Edge {
	int to, weight;

	friend std::ostream & operator<< (std::ostream & out, Edge const & edge) {
		out << " -" << edge.weight << "-> " << edge.to;
		return out;
	}
};

int main(int argc, char* argv[])
{
	if ((argc != 2) && (argc != 4)) {
		std::cout << "Usage: metis INPUT_FILE| CLICK SIZE REPs" << std::endl;
		return 1;
	}

	// Since sizeof(idx_t) == 4, we will be using integers all the way :(

	int rank, p;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &p);

	unsigned vertex_count = (argc == 4) ? (unsigned) std::stoul(argv[2]) : GraphInputIterator(argv[1]).vertexCount();
	unsigned reps = (argc == 4) ? (unsigned) std::stoul(argv[3]) : 1;



	// Build the ducky representation
	// Partition vertices evenly among ranks
	std::vector<int> vertices_per_processor(p, vertex_count  / p);
	vertices_per_processor.back() = vertex_count / p + vertex_count  % p;
	std::vector<int> vtxdist = MPIUtils::prefix_intervals(vertices_per_processor);


	// Poor man's distributed adjacency list
	int min_vertex = vtxdist.at(rank), max_vertex = vtxdist.at(rank + 1); // Exclusive range
	std::vector<std::vector<Edge>> adjacencies(max_vertex - min_vertex);

	if (argc == 4) {
		unsigned s = 10;

		std::mt19937 gen(static_cast<unsigned>(0));
		std::normal_distribution<float> mates(8, 4);
		std::normal_distribution<float> non_mates(4, 4);

		for (int i = 0; i < vertex_count; i++) {
			for (int j = i + 1; j < vertex_count; j++) {
				int weight = std::max(0.f, i % s == j % s ? mates(gen) : non_mates(gen));

				if ((min_vertex <= i) && (i < max_vertex)) {
					adjacencies.at(i - min_vertex).push_back({ j, weight });
				}
				if ((min_vertex <= j) && (j < max_vertex)) {
					adjacencies.at(j - min_vertex).push_back({ i, weight });
				}
			}
		}
	} else {
		GraphInputIterator input(argv[1]);
		//	std::cout << vtxdist << std::endl;

		for (auto edge : input) {
			// We have to duplicate edges since M assumes bidirectional graph
			if ((min_vertex <= edge.from) && (edge.from < max_vertex)) {
				adjacencies.at(edge.from - min_vertex).push_back({(int) edge.to, (int) edge.weight});
			}
			if ((min_vertex <= edge.to) && (edge.to < max_vertex)) {
				adjacencies.at(edge.to - min_vertex).push_back({(int) edge.from, (int) edge.weight});
			}
		}
	}

//	for (size_t from = 0; from < adjacencies.size(); from++) {
//		std::cout << from + min_vertex << ": " << adjacencies.at(from) << std::endl;
//	}

	// Build adjacency offset listing
	std::vector<int> adj_sizes;
	for (auto const & adj : adjacencies) {
		adj_sizes.push_back(adj.size());
	}

	std::vector<int> xadj = MPIUtils::prefix_intervals(adj_sizes);

//	std::cout << rank << " -- xadj: " << xadj << std::endl;

	// Dump adjacencies
	std::vector<int> adjncy, adjwgt;

	for (auto const & adj : adjacencies) {
		for (auto edge : adj) {
			adjncy.push_back(edge.to);
			adjwgt.push_back(edge.weight);
		}
	}

//	std::cout << rank << " -- adjncy: " << adjncy << std::endl;

	int num_cut_edges;

	// No weight at every vertex doesnt work as documented -- assign one
	std::vector<float> tpwgts { 1.0/2, 1.0/2 };
	std::vector<float> ubvec { std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
	std::vector<int> part(adjacencies.size());

	int zero = 0, one = 1, two = 2;
	int zeroes[] = { 0, 0, 0 };
	// Faceplam
	MPI_Comm comm = MPI_COMM_WORLD;


	for (unsigned i = 0; i < reps; i++) {
		double time;
		unsigned long long global_crossing_weight;
		MPI_Barrier(MPI_COMM_WORLD);
		TimeUtils::measure<void>([&]() {
			// Note that this has C null convention
			ParMETIS_V3_PartKway(
					vtxdist.data(), // vertices partial sums at processors
					xadj.data(), // xadj
					adjncy.data(), // adjacencies
					NULL, // vertex weights
					adjwgt.data(), // edge weights
					&one,    // only edge weights
					&zero, // C style indexing
					&one, // No vertex weights
					&two, // nparts 1-way cut
					tpwgts.data(), //balance constraints
					ubvec.data(), // ubvec -- arbitrary imbalance
					zeroes, // use default options -- i dont wanna know
					&num_cut_edges, // resulting cut size
					part.data(), // result vertex assignment
					&comm
			);

			// Exchange global assignment to all processors so that they can look up all their edges

			int local_size = part.size();
			std::vector<int> local_sizes(p);
			MPI_Allgather(&local_size, 1, MPI_INT, local_sizes.data(), 1, MPI_INT, MPI_COMM_WORLD);

			std::vector<int> offsets = MPIUtils::prefix_offsets(local_sizes);
			std::vector<int> assignment(vertex_count);

			MPI_Allgatherv(
					part.data(),
					local_size,
					MPI_INT,
					assignment.data(),
					local_sizes.data(),
					offsets.data(),
					MPI_INT,
					MPI_COMM_WORLD
			);

			// Now recover the cut value
			// We really have to do it for graphs of our size -- metis is stupid and overflows its own counter
			unsigned long long local_crossing_weight = 0;
			for (unsigned from = 0; from < adjacencies.size(); from++) {
				for (Edge e : adjacencies.at(from)) {
					if (assignment.at(from + min_vertex) != assignment.at(e.to)) {
						local_crossing_weight += e.weight;
					}
				}
			}

//			std::cout << local_crossing_weight << std::endl;
			MPI_Reduce(&local_crossing_weight, &global_crossing_weight, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

		}, time);

		if (rank == 0) {
			std::cout << std::fixed;
			std::cout << argv[1] << ","
					  << global_crossing_weight / 2 << "," // Every edge was counted from each endpoint
					  << time << std::endl;
		}
	}

	MPI_Finalize();
}
