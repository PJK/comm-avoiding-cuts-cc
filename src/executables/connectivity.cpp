#include "input/GraphInputIterator.hpp"
#include "../utils.hpp"
#include "../src/DisjointSets.hpp"
#include <unordered_map>
#include <random>


int main(int argc, char* argv[])
{
	if (argc != 2 && argc != 3) {
		std::cout << "Usage: connectivity INPUT_FILE [--simple]" << std::endl;
		return 1;
	}

	GraphInputIterator input(argv[1]);
	DisjointSets<unsigned> union_find(input.vertexCount());


	unsigned real_edge_count = 0;
	for (auto edge : input) {
		assert(edge.from < input.vertexCount());
		assert(edge.to < input.vertexCount());

		if (edge.to != edge.from) {
			union_find.unify(edge.from, edge.to);
			real_edge_count++;
		}
	}

	std::unordered_map<unsigned, unsigned> components;

	for (unsigned i = 0; i < input.vertexCount(); i++) {
		unsigned rep = union_find.find(i);
		if (components.find(rep) == components.end()) {
			components.insert(std::make_pair(rep, 1));
		} else {
			components.at(rep)++;
		}
	}

	if (argc == 3) {
		std::cout << components.size() << std::endl;
		return 0;
	}

	auto max_component = components.begin();

	for (auto component = components.begin(); component != components.end(); component++) {
		if (component->second > max_component->second) {
			max_component = component;
		}
	}


	std::vector<unsigned> max_component_members;
	for (unsigned i = 0; i < input.vertexCount(); i++) {
		if (union_find.find(i) == max_component->first) {
			max_component_members.push_back(i);
		}
	}


	std::random_device rd;
	std::mt19937 rng(rd());
	std::uniform_int_distribution<int> uni(0, max_component_members.size());

	std::cout << "# Graph connector tool" << std::endl;
	std::cout << input.vertexCount() << " " << real_edge_count + (input.vertexCount() - max_component->second) << std::endl;
	for (unsigned i = 0; i < input.vertexCount(); i++) {
		if (union_find.find(i) != max_component->first) {
			// Suggest a random edges
			std::cout << i << " " << max_component_members.at(uni(rng)) << " 5000" << std::endl;
		}
	}

	input.reopen();
	for (auto edge : input) {
		if (edge.to != edge.from) {
			std::cout << edge.from << " " << edge.to << " " << edge.weight << std::endl;
		}
	}
}
