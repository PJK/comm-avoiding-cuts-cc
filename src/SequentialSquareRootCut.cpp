#include "SequentialSquareRootCut.hpp"
#include "SequentialKargerSteinCut.hpp"

AdjacencyListGraph::Weight SequentialSquareRootCut::compute() {

	if (target_size_ < graph_->vertex_count()) {
		iteratedSampling(target_size_);
	}

	std::unique_ptr<AdjacencyListGraph> shrunk_graph = graph_->compact();
    
	long seed = random_->operator()();

    SequentialKargerSteinCut ks_cut(shrunk_graph.get(), seed);
    
	unsigned long ks_min = ks_cut.compute();

	return ks_min;
}

void SequentialSquareRootCut::iteratedSampling(unsigned final_size) {
	assert(graph_->edge_count() >= graph_->vertex_count() - 1);
	assert(graph_->vertex_count() >= final_size);

	// TODO assert connectivity
	while (graph_->vertex_count() > final_size) {
		AdjacencyListGraph::EdgeList edges = graph_->edges();
		std::vector<AdjacencyListGraph::Weight> weights;

		std::transform(edges.begin(), edges.end(), std::back_inserter(weights), [](const AdjacencyListGraph::Edge e) { return e.weight; });
		assert(weights.size() == edges.size());
		assert(edges.size() > 0);

		sum_tree<AdjacencyListGraph::Weight> cumulative_weights(weights.data(), weights.size());

		// XXX We always want the maximum sample size to eliminate heavy edges quickly
		unsigned sample_size { graph_->vertex_count()};

		std::vector<AdjacencyListGraph::EdgeList::size_type> edges_to_contract;
		edges_to_contract.reserve(sample_size);

		// Distribution of the weights
		std::uniform_int_distribution<AdjacencyListGraph::Weight> uniform_int(AdjacencyListGraph::Weight(1), AdjacencyListGraph::Weight(cumulative_weights.root()));

		while (edges_to_contract.size() < sample_size) {
			// Choose an edge, regardless of whether it has been chosen previously
			edges_to_contract.push_back(cumulative_weights.lower_bound(uniform_int(*random_)));
		}

		// Contract the edges
		for (auto index : edges_to_contract) {
			if (graph_->vertex_count() == final_size)
				break;

			auto edge = edges.at(index);
			// Some of these edges might have already been contracted - the representation is ok with this
			graph_->weaklyContractEdge(edge.from, edge.to);
		}

		// Remove loops and pedges
		graph_->finalizeContractionPhase(random_);
	}

//	std::cout << graph_->vertex_count() << std::endl;
//	std::cout << final_size << std::endl;
	assert(graph_->vertex_count() == final_size);
}
