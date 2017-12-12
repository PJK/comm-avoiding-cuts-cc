#ifndef PARALLEL_MINIMUM_CUT_ITERATEDSPARSESAMPLING_HPP
#define PARALLEL_MINIMUM_CUT_ITERATEDSPARSESAMPLING_HPP

#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <mpi.h>

#include "sum_tree.hpp"
#include "prng_engine.hpp"
#include "DisjointSets.hpp"
#include "sorting/SamplingSorter.hpp"
#include "MPIDatatype.hpp"
#include "recursive-contract/graph_slice.hpp"
#include "input/GraphInputIterator.hpp"
#include "utils.hpp"
#include "MPICollector.hpp"

/**
 * Implements ISS primitives
 *
 * This class performs the logic of a node within a group that is specified by the communicator.
 *
 * EdgeT must have publicly accessible properties
 * `unsigned from, to`
 * and be have a corresponding `MPIDatatype<EdgeT>` instantiation
 */
template<typename EdgeT>
class IteratedSparseSampling {
protected:

	const int root_rank_ = 0;

	MPI_Comm communicator_;
	int rank_, color_, group_size_;
	std::vector<EdgeT> edges_slice_;
	const float epsilon_ = 0.1f;
	int32_t seed_with_offset_;
	sitmo::prng_engine random_engine_;
	MPI_Datatype mpi_edge_t_;
	unsigned target_size_;
	unsigned vertex_count_;
	unsigned initial_vertex_count_, initial_edge_count_;

public:

	IteratedSparseSampling(MPI_Comm communicator,
						   int color,
						   int group_size,
						   int32_t seed_with_offset,
						   unsigned target_size,
						   unsigned vertex_count,
						   unsigned edge_count) :
			communicator_(communicator),
			color_(color),
			group_size_(group_size),
			seed_with_offset_(seed_with_offset),
			random_engine_(seed_with_offset),
			target_size_(target_size),
			vertex_count_(vertex_count),
			initial_vertex_count_(vertex_count),
			initial_edge_count_(edge_count)
	{
		MPI_Comm_rank(communicator_, &rank_);
		mpi_edge_t_ = MPIDatatype<EdgeT>::constructType();
	}

	~IteratedSparseSampling() {
	}

	int rank() const {
		return rank_;
	}

	bool master() const {
		return rank_ == root_rank_;
	}

	/**
	 * \return Is this the last node in its group?
	 */
	bool last() const {
		return rank_ == group_size_ - 1;
	}

	/**
	 * Read a ~ 1/(group size) slice of edges
	 */
	virtual void loadSlice() = 0;

	/**
	 * Send our slice to equivalent ranks in other groups
	 */
	void broadcastSlice(MPI_Comm equivalence_comm) {
		unsigned slice_size = edges_slice_.size();

		MPI::Bcast(
				&slice_size,
				1,
				MPI_UNSIGNED,
				0,
				equivalence_comm
		);

		MPI::Bcast(
				edges_slice_.data(),
				edges_slice_.size(),
				mpi_edge_t_,
				0,
				equivalence_comm
		);
	}

	/**
	 * Receive our slice
	 */
	void receiveSlice(MPI_Comm equivalence_comm) {
		unsigned slice_size;

		MPI::Bcast(
				&slice_size,
				1,
				MPI_UNSIGNED,
				0,
				equivalence_comm
		);

		edges_slice_.resize(slice_size);

		MPI::Bcast(
				edges_slice_.data(),
				edges_slice_.size(),
				mpi_edge_t_,
				0,
				equivalence_comm
		);
	}

	void setSlice(std::vector<EdgeT> edges) {
		edges_slice_ = edges;
	}

	/**
	 * Maps edge endpoints after contraction.
	 * @param vertex_map The root must contain a valid vertex mapping to apply.
	 *                   vertex_map must be of the right size (number of vertices before applying the mapping).
	 */
	void receiveAndApplyMapping(std::vector<unsigned> & vertex_map) {
		MPI::Bcast(vertex_map.data(), vertex_map.size(), MPI_UNSIGNED, root_rank_, communicator_);

		applyMapping(vertex_map);
		MPI::Bcast(&vertex_count_, 1, MPI_UNSIGNED, root_rank_, communicator_);
	}

	/**
	 * Apply the map to all endpoints, dropping loops
	 * @param vertex_map
	 */
	void applyMapping(const std::vector<unsigned> & vertex_map) {
		std::vector<EdgeT> updated_edges;

		for (auto edge : edges_slice_) {
			edge.from = vertex_map.at(edge.from);
			edge.to = vertex_map.at(edge.to);
			if (edge.from != edge.to) {
				updated_edges.push_back(edge);
			}
		}

		edges_slice_.swap(updated_edges);
	}

	/**
	 * @param edges array of {edge_count >= 0} edges
	 * @param [out] vertices_map preallocated map of {vertex_count >= 0} vertices. Will be filled with partitions label from [0, vertex_count)
	 * @param components_count the desired number of connected components
	 * @param [out] resulting_vertex_count how many vertices remain
	 * @param true if the described prefix exists
	 * I don't trust this code, it has just been copied over -- My past self has written it
	 */
	bool prefixConnectedComponents(const std::vector<EdgeT> & edges,
								   std::vector<unsigned> & vertex_map,
								   unsigned components_count,
								   unsigned & resulting_vertex_count)
	{
		DisjointSets<unsigned> dsets(vertex_map.size());

		if (components_count == 0 || vertex_map.size() == 0) {
			return true;
		}

		unsigned components_active = vertex_map.size();
		bool found = false;

		size_t i = 0;
		for (; i < edges.size() && components_active > components_count; i++) {
			int v1_set = dsets.find(edges.at(i).from),
					v2_set = dsets.find(edges.at(i).to);
			if (v1_set != v2_set) {
				components_active--;
				dsets.unify(v1_set, v2_set);
			}
		}

		if (components_active == components_count) {
			found = true;
		}

		// Also relabel the components to be in [0, new_vertex_count)!
		const long mapping_undefined = -1l;
		std::vector<long> component_labels(vertex_map.size(), mapping_undefined);
		size_t next_label = 0;
		for (unsigned j = 0; j < vertex_map.size(); j++) {
			if (component_labels.at(dsets.find(j)) == mapping_undefined) {
				component_labels.at(dsets.find(j)) = next_label++;
			}

			vertex_map.at(j) = component_labels.at(dsets.find(j));
		}

		resulting_vertex_count = components_active;
		return found;
	}

	/**
	 * Sample `edge_count` edges locally, prop. to their weight
	 * @param edge_count
	 * @return The edge sample
	 */
	virtual std::vector<EdgeT> sample(unsigned edge_count) = 0;

	unsigned initiateSampling(std::vector<int> edges_per_processor, std::vector<unsigned> & vertex_map) {
		unsigned number_of_edges_to_sample = std::accumulate(edges_per_processor.begin(), edges_per_processor.end(), 0u);

		/**
		 * Scatter sampling requests
		 */
		int edges_to_sample_locally;
		MPI::Scatter(edges_per_processor.data(), 1, MPI_INT, &edges_to_sample_locally, 1, MPI_INT, 0, communicator_);

		/**
		 * Take part in sampling
		 */
		std::vector<EdgeT> samples = sample(edges_to_sample_locally);

		/**
		 * Gather samples
		 */
		// Allocate space
		std::vector<EdgeT> global_samples(number_of_edges_to_sample);
		// Calculate displacement vector
		std::vector<int> relative_displacements(edges_per_processor);
		relative_displacements.insert(relative_displacements.begin(), 0); // Start at offset zero
		relative_displacements.pop_back(); // Last element not needed

		std::vector<int> displacements;
		std::partial_sum(relative_displacements.begin(), relative_displacements.end(), std::back_inserter(displacements));

		assert(master());
		MPI::Gatherv(
				samples.data(),
				edges_to_sample_locally,
				mpi_edge_t_,
				global_samples.data(),
				edges_per_processor.data(),
				displacements.data(),
				mpi_edge_t_,
				root_rank_,
				communicator_
		);

		assert(global_samples.size() == number_of_edges_to_sample);

		/**
		 * Shuffle to ensure random order for the prefix
		 */
		std::shuffle(global_samples.begin(), global_samples.end(), random_engine_);

		/**
		 * Incremental prefix scan
		 */
		vertex_map.resize(vertex_count_);
		unsigned resulting_vertex_count;
		prefixConnectedComponents(
				global_samples,
				vertex_map,
				target_size_,
				resulting_vertex_count
		);

		vertex_count_ = resulting_vertex_count;
		// Yay we are done
		return resulting_vertex_count;
	}

	/**
	 * Match `initiateSampling` at non-root nodes
	 */
	void acceptSamplingRequest() {
		int edges_to_sample_locally;
		MPI::Scatter(nullptr, 1, MPI_INT, &edges_to_sample_locally, 1, MPI_INT, 0, communicator_);

		std::vector<EdgeT> samples = sample(edges_to_sample_locally);

		MPI::Gatherv(
				samples.data(),
				edges_to_sample_locally,
				mpi_edge_t_,
				nullptr,
				nullptr,
				nullptr,
				mpi_edge_t_,
				root_rank_,
				communicator_
		);
	}

	unsigned vertexCount() const {
		return vertex_count_;
	}

	unsigned initialEdgeCount() const {
		return initial_edge_count_;
	}
};

#endif //PARALLEL_MINIMUM_CUT_ITERATEDSPARSESAMPLING_HPP
