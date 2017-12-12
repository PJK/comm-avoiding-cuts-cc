#ifndef PARALLEL_MINIMUM_CUT_SAMPLINGSORTER_HPP
#define PARALLEL_MINIMUM_CUT_SAMPLINGSORTER_HPP

#include "mpi.h"
#include "../../lib/prng_engine.hpp"
#include <vector>
#include <cmath>
#include <random>
#include <algorithm>
#include <cassert>
#include "MPIDatatype.hpp"
#include "utils.hpp"
#include "MPICollector.hpp"

template <typename ElementType>
class SamplingSorter {
	MPI_Comm communicator_;
	int p_, rank_;
	MPI_Datatype element_type_;
	std::vector<ElementType> data_;
	sitmo::prng_engine random_engine_;



public:
	/**
	 * The communicator ownership is 'transfered' to the sorter until all members have performed `sort`
	 */
	SamplingSorter(MPI_Comm communicator, std::vector<ElementType> && data, int32_t seed_with_offset) :
			communicator_(communicator),
			data_(data),
			random_engine_(seed_with_offset)
	{
		MPI_Comm_size(communicator_, &p_);
		MPI_Comm_rank(communicator_, &rank_);
		element_type_ = MPIDatatype<ElementType>::constructType();
	}

	/**
	 * Execute the sort.
	 * Note: all sizes are communicated as integers for simplicity (size_t is platform-specific)
	 * TODO: possibly use unsigneds or define the mapping
	 */
	std::vector<ElementType> sort() {
		/*
		 * ======== Exchange metadata ========
		 */
		/** Total size */
		int n, local_elements = data_.size();
		MPI::Allreduce(&local_elements, &n, 1, MPI_INT, MPI_SUM, communicator_);

		/*
		 * ======== Sample locally ========
		 */
		double sampling_probability = std::pow(double(n), -0.5);
		std::uniform_real_distribution<double> distribution(0, 1);

		std::vector<ElementType> local_samples;
		for (auto const & element : data_) {
			if (distribution(random_engine_) <= sampling_probability) {
				local_samples.push_back(element);
			}
		}

		/*
		 * ======== Communicate samples ========
		 */
		// Communicate all sample sizes, we will need a non-uniform all-gather
		int local_sample_size = local_samples.size();
		std::vector<int> sample_sizes(p_);
		MPI::Allgather(&local_sample_size, 1, MPI_INT, sample_sizes.data(), 1, MPI_INT, communicator_);

		std::vector<int> samples_offsets = MPIUtils::prefix_offsets(sample_sizes);
		int total_sample_size = samples_offsets.back() + sample_sizes.back();

		/** Global samples to be gathered */
		std::vector<ElementType> samples(total_sample_size);

		MPI::Allgatherv(
				local_samples.data(),
				local_sample_size,
				element_type_,
				samples.data(),
				sample_sizes.data(),
				samples_offsets.data(),
				element_type_,
				communicator_
		);

		/*
		 * ======== Sort and partition elements ========
		 */
		std::sort(samples.begin(), samples.end());
		std::sort(data_.begin(), data_.end());

		/** Number of samples per processor */
		int k = total_sample_size / p_;
		/**
		 * Select pivots P_1, ..., P_{p - 1} for division among processors.
		 * p_1 will get all edges e | e < P_1
		 * p_2 will get all edges e | P_1 <= e < P_2
		 * ...
		 * p_p will get all edges e | P_{p - 1} <= e
		 * Note that C++ orders are defined by `operator<`
		 */
		std::vector<ElementType> pivots;
		for (int i { 1 }; i < p_; i++) {
			pivots.push_back(samples.at(i * k));
		}

		/** p_i will receive elements_per_processor[p_i] elements */
		std::vector<int> elements_per_processor(p_, 0);
		// Build logical partitioning of elements among processors
		/** How many elements are yet to be assigned */
		int elements_remaining = data_.size();
		/** Next element to be assigned */
		auto current_element = data_.begin();

		for (unsigned pivot_ordinal { 0 };
			 pivot_ordinal < pivots.size();
			 pivot_ordinal++) {
			while ((current_element != data_.end()) && (*current_element < pivots.at(pivot_ordinal))) {
				elements_per_processor.at(pivot_ordinal)++;
				elements_remaining--;
				++current_element;
			}
		}

		// All remaining elements go to the last processor
		assert(elements_per_processor.at(p_ - 1) == 0);
		elements_per_processor.at(p_ - 1) = elements_remaining;

		// Assert that all elements have been assigned somewhere
		assert(std::accumulate(elements_per_processor.begin(), elements_per_processor.end(), 0) == (int) data_.size());

		/*
		 * ======== Send elements to their processors ========
		 */
		// Communicate the number of elements that will be send from each processor to each other processor
		/** This processor will receive elements_to_receive[i] elements from p_i */
		std::vector<int> elements_to_receive(p_);
		MPI::Alltoall(
				elements_per_processor.data(),
				1,
				MPI_INT,
				elements_to_receive.data(),
				1,
				MPI_INT,
				communicator_
		);

		// Now we can calculate the offsets for non-uniform receive
		/** Displacements of the groups we will receive */
		std::vector<int> arriving_elements_groups_offsets = MPIUtils::prefix_offsets(elements_to_receive);
		/** Displacements of our element groups */
		std::vector<int> elements_groups_offsets = MPIUtils::prefix_offsets(elements_per_processor);

		/** How many elements we will get in total */
		int number_of_elements_to_receive = std::accumulate(elements_to_receive.begin(), elements_to_receive.end(), 0);
		/** Target buffer */
		std::vector<ElementType> partitioned_data_slice(number_of_elements_to_receive);

		// Yay finally
		MPI::Alltoallv(
				data_.data(),
				elements_per_processor.data(),
				elements_groups_offsets.data(),
				element_type_,
				partitioned_data_slice.data(),
				elements_to_receive.data(),
				arriving_elements_groups_offsets.data(),
				element_type_,
				communicator_
		);

		std::sort(partitioned_data_slice.begin(), partitioned_data_slice.end());

		// Et voila!
		return partitioned_data_slice;
	}
};


#endif //PARALLEL_MINIMUM_CUT_SAMPLINGSORTER_HPP
