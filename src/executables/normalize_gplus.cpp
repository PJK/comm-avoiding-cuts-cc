#include <unordered_map>
#include <stdio.h>
#include <inttypes.h>

std::unordered_map<uint_fast64_t, uint_fast64_t> vertex_labels;
size_t vertex_counter = 0, edge_counter = 0;

uint_fast64_t translate(uint_fast64_t vertex) {
	if (vertex_labels.find(vertex) != vertex_labels.end()) {
		return vertex_labels.at(vertex);
	} else {
		vertex_labels[vertex] = vertex_counter;
		return vertex_counter++;
	}
}

int main(int argc, char* argv[]) {
	uint_fast64_t from, to;

	// CPP IO fails again
	while(scanf("%" PRIu64 " %" PRIu64 "\n", &from, &to) != EOF) {
		fprintf(stderr, "%" PRIu64 " %" PRIu64 "\n", from, to);
		edge_counter++;
		if (edge_counter % 1000000 == 0) {
			fprintf(stderr, "%zu\n", edge_counter);
		}

		printf("%" PRIu64 " %" PRIu64 " 1\n", translate(from), translate(to));
	}

	fprintf(stderr, "Add the following at the beginning of the file:\n");
	fprintf(stderr, "# Comment\n");
	fprintf(stderr, "%zu %zu\n", vertex_counter, edge_counter);
}
