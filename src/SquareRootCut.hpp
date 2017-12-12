#ifndef PARALLEL_MINIMUM_CUT_SQUAREROOTCUT_HPP
#define PARALLEL_MINIMUM_CUT_SQUAREROOTCUT_HPP

#include "mpi.h"
#include "AdjacencyListGraph.hpp"
#include "SequentialSquareRootCut.hpp"
#include "GraphInputIterator.hpp"
#include "WeightedIteratedSparseSampling.hpp"
#include "FileIteratedSampling.hpp"
#include "CLICKIteratedSampling.hpp"

/**
 * Implements the sqrt(n) `sparse' minimum cut algorithm. This is the top level class
 * that abstracts away the algorithm implementation as well as MPI details.
 */
class SquareRootCut {
	MPI_Comm communicator_;
	int p_, rank_;
	MPI_Datatype mpi_edge_t_;
	double base_case_multiplier_;
	static const int odd_color_ = std::numeric_limits<int>::max();
	void initializeDatatype();
	typedef graph_slice<long> GraphSlice;
	unsigned vertex_count_, initial_edge_count_;

public:
	/**
	 * HC minimum group size. A power of 2
	 */
	static const unsigned group_size_ = 2;

	enum Variant { LOW_CONCURRENCY, HIGH_CONCURRENCY };
	struct Result {
		AdjacencyListGraph::Weight weight;
		Variant variant;
		unsigned trials;
		double cuttingTime, mpiTime;
	};

	/**
	 * The communicator ownership is exclusive to SquareRootCut until all members have performed
	 * a runMaster/runWorker.
	 */
	SquareRootCut(MPI_Comm comm, double base_case_multiplier = 2) :
			communicator_(comm),
			base_case_multiplier_(base_case_multiplier)
	{
		MPI_Comm_size(communicator_, &p_);
		MPI_Comm_rank(communicator_, &rank_);
		base_case_multiplier_ = base_case_multiplier;
	}

	/**
	 * Sequential version constructor. Attempting to run any function except for `seqMaster`
	 * is illegal and will result in undefined behavior.
	 *
	 * @param base_case_multiplier
	 */
	SquareRootCut(double base_case_multiplier = 2) :
			base_case_multiplier_(base_case_multiplier)
	{}

	bool master() const {
		return rank_ == 0;
	}

	unsigned processors() const {
		return (unsigned) p_;
	}

	unsigned initialVertexCount() const {
		return vertex_count_;
	}

	unsigned initialEdgeCount() const {
		return initial_edge_count_;
	}

	/**
	 * \return Are the groups singular?
	 */
	bool lowConcurrency(unsigned vertex_count, unsigned edge_count, double success_probability) const;

	Result runMaster(GraphInputIterator & input, double success_probability, uint32_t seed);

	void runWorker(GraphInputIterator & input, double success_probability, uint32_t seed);

	Result runClickMaster(unsigned vertex_count, double success_probability, uint32_t seed);

	void runClickWorker(unsigned vertex_count, double success_probability, uint32_t seed);

	/**
	 * \param n number of vertices
	 * \param m number of edges
	 * \param success_probability Minimum success probability
	 */
	unsigned numberOfTrials(unsigned n, unsigned m, double success_probability) const;

	Result seqMaster(GraphInputIterator & input, double success_probability, uint32_t seed);

protected:

	/**
	 * \param success_probability Minimum success probability
	 */
	double cPrime(double success_probability) const;

	unsigned intermediate_size(unsigned n, unsigned m) const;

	/**
	 * \param graph
	 * \param success_probability Minimum success probability
	 * \param seed                PRNG seed. Workers will be seeded with {seed + process_index}
	 * \return 2-tuple of (minimum cut weight, number of trials per processor)
	 */
	Result runLowConcurrencyMaster(GraphInputIterator & input, double success_probability, uint32_t seed);

	void runLowConcurrencyWorker();

	/**
	 * Wrapper to parametrize runners
	 */

	class SamplerFactory {
	public:
		unsigned vertex_count_, edge_count_;

		SamplerFactory(unsigned vertex_count, unsigned edge_count) : vertex_count_(vertex_count), edge_count_ (edge_count)
		{}

		virtual std::unique_ptr<WeightedIteratedSparseSampling> build(MPI_Comm communicator, int color, int group_size, int32_t seed_with_offset, unsigned target_size) = 0;
	};

	class FileSamplerFactory : public SamplerFactory {
	public:
		GraphInputIterator & input_;

		FileSamplerFactory(GraphInputIterator & input) : SamplerFactory(input.vertexCount(), input.edgeCount()), input_(input)
		{}

		virtual std::unique_ptr<WeightedIteratedSparseSampling> build(MPI_Comm communicator, int color, int group_size, int32_t seed_with_offset, unsigned target_size) {
			return std::unique_ptr<WeightedIteratedSparseSampling>(new FileIteratedSampling(communicator, color, group_size, input_, seed_with_offset, target_size));
		}
	};

	class ClickSamplerFactory : public SamplerFactory {
	public:
		using SamplerFactory::SamplerFactory;

		virtual std::unique_ptr<WeightedIteratedSparseSampling> build(MPI_Comm communicator, int color, int group_size, int32_t seed_with_offset, unsigned target_size) {
			return std::unique_ptr<WeightedIteratedSparseSampling>(new CLICKIteratedSampling(communicator, color, group_size, seed_with_offset, vertex_count_, target_size));
		}
	};

	/**
	 * Let all nodes arrange into groups. The groups will have a 'local' master that will guide the shrinking.
	 */
	Result participateInGroup(SamplerFactory & sampler, double success_probability, uint32_t seed);

	Result runConcurrentMaster(SamplerFactory & sampler, double success_probability, uint32_t seed);

	void runConcurrentWorker(SamplerFactory & sampler, double success_probability, uint32_t seed);
};


#endif //PARALLEL_MINIMUM_CUT_SQUAREROOTCUT_HPP
