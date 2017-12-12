#include <iostream>
#include <string>
#include <vector>
#include "Galois/Galois.h"
#include "Galois/Graph/FileGraph.h"
#include "input/GraphInputIterator.hpp"
#include "AdjacencyListGraph.hpp"

int main(int argc, char* argv[]) {
	GraphInputIterator input(argv[1]);

	Galois::Graph::FileGraphWriter writer;
	writer.setNumNodes(input.vertexCount());
	writer.setNumEdges(2 * input.edgeCount());
	writer.setSizeofEdgeData(0);

	std::cout << "Phase1 starting" << std::endl;
	writer.phase1();

	for (auto edge : input) {
		writer.incrementDegree(edge.from, 1);
		writer.incrementDegree(edge.to, 1);
	}

	std::cout << "Created degrees" << std::endl;

	input.reopen();

	writer.phase2();

	std::cout << "Phase2 starting" << std::endl;

	for (auto edge : input) {
		writer.addNeighbor(edge.from, edge.to);
		writer.addNeighbor(edge.to, edge.from);
	}

	std::cout << "Added neighbors" << std::endl;

	writer.finish<void>();

	writer.structureToFile(argv[2]);
}
