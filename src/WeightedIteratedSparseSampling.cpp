#include <stdexcept>
#include "WeightedIteratedSparseSampling.hpp"

void WeightedIteratedSparseSampling::shrink() {
	while (!samplingTrial()) {}
}

bool WeightedIteratedSparseSampling::samplingTrial() {

	std::vector<AdjacencyListGraph::Weight> weights;
	gatherWeights(weights);

	std::vector<unsigned> vertex_map(vertex_count_);

	if (master()) {
		initiateSampling(edgesToSamplePerProcessor(weights), vertex_map);
	} else {
		acceptSamplingRequest();
	}

	receiveAndApplyMapping(vertex_map);

	return vertex_count_ == target_size_;
}

graph_slice<long> WeightedIteratedSparseSampling::reduce() {
	// To enable correct sorting
	std::for_each(
			edges_slice_.begin(),
			edges_slice_.end(),
			[](AdjacencyListGraph::Edge & e) { e.normalize(); }
	);

	SamplingSorter<AdjacencyListGraph::Edge> sorter(communicator_, std::move(edges_slice_), seed_with_offset_);
	std::vector<AdjacencyListGraph::Edge> sorted_slice = sorter.sort();

	// Fun part: reduce edges
	// Lukas: This is what you were looking for?
	// Note that MPI_Scan is not directly applicable since it produces the same number of outputs, and there
	// is no simple way to have 'carry' and still mark all edges but one for each pair of vertices void
	std::vector<AdjacencyListGraph::Edge> locally_reduced_slice;
	if (! sorted_slice.empty()) {
		auto sorted_edges_iterator = sorted_slice.begin();
		locally_reduced_slice.push_back(*(sorted_edges_iterator++));
		while (sorted_edges_iterator != sorted_slice.end()) {
			if (locally_reduced_slice.back() == *sorted_edges_iterator) {
				locally_reduced_slice.back().weight += sorted_edges_iterator->weight;
			} else {
				locally_reduced_slice.push_back(*sorted_edges_iterator);
			}
			++sorted_edges_iterator;
		}
	}

	/*
	 * Now it gets even funnier. We need to merge the edges on the boundaries of slices.
	 * Slice can be either:
	 *  - empty
	 *  - singular
	 *  - have two or more distinct edges (regular)
	 *
	 * There can be a segment of singular processor of length up to p:
	 * ... || (a, b) || (a, b) || .... || (a, b) || (c, d) || ...
	 * Moreover, there may be any number of empty processors at the end.
	 *
	 * How we deal with this:
	 * Every processor 'offers' it's first edge to the preceding one. These edges are all-gathered.
	 * Processors that don't have an edge to offer will supply a dummy value.
	 *
	 * If a processor was offered a dummy edge, it ignores it.
	 *
	 * All processors that offered an edge except the first one remote that edge from their slice.
	 *
	 * Now we consider processors that were offered a real edge. Since it may be that there are
	 * several following processors that offer the same value, the leftmost processor will collect
	 * all the edges that should be combined with the first edge it receives
	 *
	 * Note that there might be gaps with processors owning no edges after this process.
	 */

	/** Boundary edges offred by each processor */
	std::vector<AdjacencyListGraph::Edge> boundary_edges(group_size_);

	/** Dummy value to signal empty slice */
	AdjacencyListGraph::Edge dummy_edge = {
			std::numeric_limits<unsigned >::max(),
			std::numeric_limits<unsigned >::max(),
			std::numeric_limits<AdjacencyListGraph::Weight>::max()
	};

	if (locally_reduced_slice.empty()) {
		locally_reduced_slice.push_back(dummy_edge);
	}

	MPI::Allgather(
			&locally_reduced_slice.front(),
			1,
			mpi_edge_t_,
			boundary_edges.data(),
			1,
			mpi_edge_t_,
			communicator_
	);

	// Give up the edge we've offered
	if (rank_ != 0) {
		locally_reduced_slice.erase(locally_reduced_slice.begin());
	}

	// If we are not the last processor and we've been offered a real edge
	if ((rank_ < (group_size_ - 1)) && (boundary_edges.at(rank_ + 1) != dummy_edge)) {
		// ... and we are the leftmost processor for this streak of edges
		if (boundary_edges.at(rank_) != boundary_edges.at(rank_ + 1)) {
			// ... then collect all the edges ...
			size_t next_edge_to_collect = rank_ + 1;

			// ... if the offered edge is different, add it explicitly ...
			if (locally_reduced_slice.back() != boundary_edges.at(next_edge_to_collect)) {
				locally_reduced_slice.push_back(boundary_edges.at(next_edge_to_collect++)); // and move on to the next edges
			}

			// ... and take all matching edges
			while ((next_edge_to_collect < size_t(group_size_)) && (boundary_edges.at(next_edge_to_collect) == locally_reduced_slice.back())) {
				locally_reduced_slice.back().weight += boundary_edges.at(next_edge_to_collect++).weight;
			}
		}
	}

	// TODO scope the locals so that we can throw away the sorted list


	/*
	 * Matrix construction time.
	 *
	 * Lukas hath said "for a matrix n' rows and a subset of currently p' processors:
	 * the matrix is distributed row-wise such that each processor holds ceil(n/p) consecutive rows of the matrix."
	 * "note also that the matrix needs to be padded with zeroes, such that p' divides the number of rows."
	 *
	 * A minor problem is that it is easy to determine where to send the edges based on the first component (they
	 * are sorted by it and thus we can compute non-uniform all-to-all displacements, but this will only initialize
	 * one half of the entries, the corresponding symmetric entries will be missing. We just add a transpose manually.
	 *
	 * TODO is there a nicer way to do it already built in?
	 */

	{
		int rows_per_processor = (int) std::ceil(float(vertex_count_) / group_size_);
		int row_col_size = rows_per_processor * group_size_;

		/** p_i will receive edges_per_processor[i] edges to place in its slice of rows */
		std::vector<int> edges_per_processor(group_size_, 0);

		// Duplicate edges to get both triangles
		{
			size_t original_size = locally_reduced_slice.size();
			for (size_t i { 0 }; i < original_size; i++) {
				AdjacencyListGraph::Edge e = locally_reduced_slice.at(i);
				std::swap(e.from, e.to);
				locally_reduced_slice.push_back(e);
			}
		}

		std::sort(locally_reduced_slice.begin(), locally_reduced_slice.end());

		for (auto const edge : locally_reduced_slice) {
			edges_per_processor.at(edge.from / rows_per_processor)++;
		}

		/** Receive displacements */
		std::vector<int> edges_to_receive(group_size_);

		/* Exchange segment sizes */
		MPI::Alltoall(
				edges_per_processor.data(),
				1,
				MPI_INT,
				edges_to_receive.data(),
				1,
				MPI_INT,
				communicator_
		);

		/** Compute offsets */
		std::vector<int> arriving_edges_offsets = MPIUtils::prefix_offsets(edges_to_receive);
		std::vector<int> edges_groups_offsets = MPIUtils::prefix_offsets(edges_per_processor);
		int number_of_edges_to_receive = std::accumulate(edges_to_receive.begin(), edges_to_receive.end(), 0);
		/** Target buffer */
		std::vector<AdjacencyListGraph::Edge> incoming_edges_buffer(number_of_edges_to_receive);

		MPI::Alltoallv(
				locally_reduced_slice.data(),
				edges_per_processor.data(),
				edges_groups_offsets.data(),
				mpi_edge_t_,
				incoming_edges_buffer.data(),
				edges_to_receive.data(),
				arriving_edges_offsets.data(),
				mpi_edge_t_,
				communicator_
		);

		// C++ it like it's 1998
		// FIXME longs
		long * rows_slice = new long[row_col_size * rows_per_processor];
		std::fill(rows_slice, rows_slice + row_col_size * rows_per_processor, 0l);

		/** Global index of our first row */
		size_t row_offset = rows_per_processor * rank_;

		for (auto edge : incoming_edges_buffer) {
			rows_slice[(edge.from - row_offset) * row_col_size + edge.to] = edge.weight;
		}

		DebugUtils::print(rank_, [&](std::ostream & out) {
			out << "About to construct slice with: vertex count: " << vertex_count_
				<< " rows_per_processor: " << rows_per_processor
				<< " row_col_size: " << row_col_size
				<< " color: " << color_;
		});

		// FIXME  longs
		graph_slice<long> graph_slice(
				vertex_count_,
				rows_per_processor,
				rank_,
				row_col_size,
				rows_slice
		);

		assert (graph_slice.invariant());

		return graph_slice;
	}
}

std::vector<AdjacencyListGraph::Edge> WeightedIteratedSparseSampling::sample(unsigned edge_count) {
	/**
	 * Preprocessing
	 */
	std::vector<AdjacencyListGraph::Weight> edge_weights;
	std::transform(
			edges_slice_.begin(),
			edges_slice_.end(),
			back_inserter(edge_weights),
			[](AdjacencyListGraph::Edge edge) { return edge.weight; }
	);
	std::vector<AdjacencyListGraph::Edge> edges;

	// <3 <3 <3 sum_tree cannot store <= sequences. The client code should handle this, obviously
	if (edges_slice_.size() <= 1) {
		if (edges_slice_.size() == 1) {
			edges.resize(edge_count, edges_slice_[0]);
		}

		return edges;
	}

	assert(edge_weights.size() == edges_slice_.size());
	sum_tree<AdjacencyListGraph::Weight> index(edge_weights.data(), edge_weights.size());

	edges.reserve(edge_count);

	// TODO possibly keep the computed weight
	std::uniform_int_distribution<AdjacencyListGraph::Weight> uniform_int(
			1,
			std::accumulate(edge_weights.begin(), edge_weights.end(), 0)
	);

	/**
	 * Sampling
	 */
	for (size_t i = 0; i < edge_count; i++) {
		edges.push_back(
				edges_slice_.at(index.lower_bound(uniform_int(random_engine_)))
		);
	}

	return edges; // NRVO
}

void WeightedIteratedSparseSampling::gatherWeights(std::vector<AdjacencyListGraph::Weight> & weights) {
	// Compute weight sums
	AdjacencyListGraph::Weight slice_weight = std::accumulate(
			edges_slice_.begin(),
			edges_slice_.end(),
			0,
			[](AdjacencyListGraph::Weight state, AdjacencyListGraph::Edge & edge) { return state + edge.weight; }
	);

	if (master()) {
		weights.resize(group_size_);
	}

#ifndef NDEBUG
	int size;
	MPI_Type_size(MPI_UNSIGNED_LONG, &size);
	assert(sizeof(AdjacencyListGraph::Weight) == size);
#endif

	MPI::Gather(&slice_weight, 1, MPI_UNSIGNED_LONG, weights.data(), 1, MPI_UNSIGNED_LONG, root_rank_, communicator_);
}


AdjacencyListGraph::Weight WeightedIteratedSparseSampling::countEdges(std::vector<AdjacencyListGraph::Weight> & weights) {
	gatherWeights(weights);

	unsigned long edges;

	if (master()) {
		edges = std::accumulate(weights.begin(), weights.end(), 0);
	}
	MPI::Bcast(&edges, 1, MPI_UNSIGNED_LONG, root_rank_, communicator_);

	return edges;
}

unsigned WeightedIteratedSparseSampling::connectedComponents(std::vector<unsigned> & cc) {
	if (vertex_count_ != initial_vertex_count_) {
		throw std::logic_error("Cannot perform CC on a shrinked graph");
	}

	std::vector<AdjacencyListGraph::Weight> weights;

	if (master()) {
		cc.resize(vertex_count_);
		for (unsigned i = 0; i < vertex_count_; i++) {
			cc.at(i) = i;
		}
	}

	while(countEdges(weights) > 0) {
		std::vector<unsigned> vertex_map(vertex_count_);

		if (master()) {
			initiateSampling(edgesToSamplePerProcessor(weights), vertex_map);

			for (size_t i = 0; i < cc.size(); i++) {
				cc.at(i) = vertex_map.at(cc.at(i));
			}
		} else {
			acceptSamplingRequest();
		}

		receiveAndApplyMapping(vertex_map);
	}

	return vertex_count_;
}
