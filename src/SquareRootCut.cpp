#include "SquareRootCut.hpp"
#include "AdjacencyListGraph.hpp"
#include "SequentialKargerSteinCut.hpp"
#include "karger-stein/co_mincut.h"
#include <limits>
#include "recursive-contract/recursive_contract.hpp"
#include "utils.hpp"
#include "MPICollector.hpp"
#include "FileIteratedSampling.hpp"

bool SquareRootCut::lowConcurrency(unsigned vertex_count, unsigned edge_count, double success_probability) const {
	return processors() < group_size_ * numberOfTrials(vertex_count, edge_count, success_probability);
}

SquareRootCut::Result SquareRootCut::runMaster(GraphInputIterator & input, double success_probability, uint32_t seed) {
	vertex_count_ = input.vertexCount();
	initial_edge_count_ = input.edgeCount();

	if (lowConcurrency(input.vertexCount(), input.edgeCount(), success_probability)) {
		return runLowConcurrencyMaster(input, success_probability, seed);
	} else {
		FileSamplerFactory fsf(input);
		return runConcurrentMaster(fsf, success_probability, seed);
	}
}

void SquareRootCut::runWorker(GraphInputIterator & input, double success_probability, uint32_t seed) {
	vertex_count_ = input.vertexCount();
	initial_edge_count_ = initialEdgeCount();

	if (lowConcurrency(input.vertexCount(), input.edgeCount(), success_probability)) {
		runLowConcurrencyWorker();
	} else {
		FileSamplerFactory fsf(input);
		runConcurrentWorker(fsf, success_probability, seed);
	}
}

SquareRootCut::Result SquareRootCut::runClickMaster(unsigned vertex_count, double success_probability, uint32_t seed) {
	vertex_count_ = vertex_count;
	initial_edge_count_ = vertex_count_ * (vertex_count_ - 1) / 2;

	if (lowConcurrency(vertex_count, initial_edge_count_, success_probability)) {
		throw "Not implemented";
	} else {
		ClickSamplerFactory csf(vertex_count, initial_edge_count_);
		return runConcurrentMaster(csf, success_probability, seed);
	}
}

void SquareRootCut::runClickWorker(unsigned vertex_count, double success_probability, uint32_t seed) {
	vertex_count_ = vertex_count;
	initial_edge_count_ = vertex_count_ * (vertex_count_ - 1) / 2;

	if (lowConcurrency(vertex_count, initial_edge_count_, success_probability)) {
		throw "Not implemented";
	} else {
		ClickSamplerFactory csf(vertex_count, initial_edge_count_);
		runConcurrentWorker(csf, success_probability, seed);
	}
}

unsigned SquareRootCut::intermediate_size(unsigned n, unsigned m) const {
	unsigned contracted_size = (unsigned) std::ceil(base_case_multiplier_ * std::sqrt(m) + 1);
	if (contracted_size > n) {
		return n;
	} else {
		return contracted_size;
	}
}

double SquareRootCut::cPrime(double success_probability) const {
	return double(1) / (1 - success_probability);
}

unsigned SquareRootCut::numberOfTrials(unsigned n, unsigned m, double success_probability) const {
	double pk_s = comincut::min_success_in_one_trial(intermediate_size(n, m));

	//TODO: WATCH OUT FOR OVERFLOWS?
	return (unsigned) std::ceil(
			(n * (n * std::log(cPrime(success_probability)))) / (base_case_multiplier_ * base_case_multiplier_ * (m * pk_s))
	);
}


SquareRootCut::Result SquareRootCut::runLowConcurrencyMaster(GraphInputIterator & input, double success_probability, uint32_t seed) {
	SquareRootCut::Result result;
	result.variant = LOW_CONCURRENCY;

	AdjacencyListGraph graph = TimeUtils::profile<AdjacencyListGraph>([&]() {
		return AdjacencyListGraph::fromIterator(input);
	}, "load_input");

	mpi_edge_t_ = MPIDatatype<AdjacencyListGraph::Edge>::constructType();

	AdjacencyListGraph::EdgeList edges = graph.edges();
	AdjacencyListGraph::Weight local_min;

	double total_trials = numberOfTrials(graph.vertex_count(), graph.edge_count(), success_probability);
	unsigned trials = (unsigned) std::ceil(total_trials / processors());

	result.trials = trials;

	// Overflow guard
	assert(trials * processors() >= total_trials);

	// We need references for MPI
	unsigned edge_count = graph.edge_count(),
			vertex_count = graph.vertex_count();

	MPI_Barrier(communicator_);
	PAPI_START();

	TimeUtils::measure<void>([&]() {
		MPI::total = 0; // Just to be sure
		// Broadcast the array length so that we can use it as parameter for the following call
		MPI::Bcast(&edge_count, 1, MPI_UNSIGNED, 0, communicator_);
		MPI::Bcast(&vertex_count, 1, MPI_UNSIGNED, 0, communicator_);
		MPI::Bcast(&trials, 1, MPI_UNSIGNED, 0, communicator_);
		MPI::Bcast(&seed, 1, MPI_UINT32_T, 0, communicator_);

			// <3 vector
		MPI::Bcast(edges.data(), edge_count, mpi_edge_t_, 0, communicator_);

		// No-copy initialization
		AdjacencyListGraph g(vertex_count, std::move(edges));
		local_min = std::numeric_limits<unsigned long>::max();

		sitmo::prng_engine engine(seed + rank_);

		unsigned t = intermediate_size(vertex_count, edge_count);

		for (unsigned i{0}; i < trials; i++) {
			TimeUtils::profile<void>([&]() {
				local_min = std::min(local_min, SequentialSquareRootCut(
						std::make_shared<AdjacencyListGraph>(AdjacencyListGraph(g)).get(), &engine, t).compute());
			}, "local_trial");
		}

		MPI::Reduce(&local_min, &result.weight, 1, MPI_UNSIGNED_LONG, MPI_MIN, 0, communicator_);
	}, result.cuttingTime);

	PAPI_STOP(rank_, MPI::total);

	MPI_Reduce(&MPI::total, &result.mpiTime, 1, MPI_DOUBLE, MPI_MAX, 0, communicator_);

	return result;
}

void SquareRootCut::runLowConcurrencyWorker() {
	MPI_Barrier(communicator_);
	PAPI_START();

	MPI::total = 0; // Just to be sure

	mpi_edge_t_ = MPIDatatype<AdjacencyListGraph::Edge>::constructType();

	AdjacencyListGraph::EdgeList edges;

	unsigned edge_count, vertex_count, trials;
	uint32_t seed;
	AdjacencyListGraph::Weight local_min, global_min;

	MPI::Bcast(&edge_count, 1, MPI_UNSIGNED, 0, communicator_);
	MPI::Bcast(&vertex_count, 1, MPI_UNSIGNED, 0, communicator_);
	MPI::Bcast(&trials, 1, MPI_UNSIGNED, 0, communicator_);
	MPI::Bcast(&seed, 1, MPI_UINT32_T, 0, communicator_);

	edges.resize(edge_count);
	MPI_Bcast(edges.data(), edge_count, mpi_edge_t_, 0, communicator_);

	AdjacencyListGraph g(vertex_count, std::move(edges));
	local_min = std::numeric_limits<unsigned long>::max();
	sitmo::prng_engine engine(seed + rank_);

	unsigned t = intermediate_size(vertex_count, edge_count);

	for (unsigned i { 0 }; i < trials; i++) {
		local_min = std::min(local_min, SequentialSquareRootCut(std::make_shared<AdjacencyListGraph>(AdjacencyListGraph(g)).get(), &engine, t).compute());
	}

	MPI::Reduce(&local_min, &global_min, 1, MPI_UNSIGNED_LONG, MPI_MIN, 0, communicator_);

	PAPI_STOP(rank_, MPI::total);
	MPI_Reduce(&MPI::total, NULL, 1, MPI_DOUBLE, MPI_MAX, 0, communicator_);
}


SquareRootCut::Result SquareRootCut::participateInGroup(SamplerFactory & samplerFactory, double success_probability, uint32_t seed) {
	SquareRootCut::Result result;
	result.variant = HIGH_CONCURRENCY;
	result.trials = 1;

	/**
	 * Create groups that each hold a copy of the graph
	 *
	 * Note that, for a fixed color, the keys need not be unique. It is MPI_COMM_SPLIT's responsibility to sort
	 * processes in ascending order according to this key, and to break ties in a consistent way. If all the keys
	 * are specified in the same way, then all the processes in a given color will have the relative rank order as
	 * they did in their parent group. (In general, they will have different ranks.)
	 *
	 * -- http://mpi.deino.net/mpi_functions/MPI_Comm_split.html
	 */

	int processors_per_trial = processors() / numberOfTrials(samplerFactory.vertex_count_, samplerFactory.edge_count_, success_probability);
	/** Take the closest lesser than or equal power of two -- the RC impl requires it */
	// TODO tweak the trials computation to lessen the impact
	int group_size = (int) std::pow(
			2,
			std::floor(std::log2(processors_per_trial))
	);
	assert(group_size >= group_size_);

	int group_count = processors() / group_size;

	MPI_Comm group_communicator, equivalent_ranks_comm;

	// There may be up to group_size - 1 odd nodes -- move them to a special group and exclude them from future processing
	if (rank_ >= group_size * group_count) {
		MPI_Comm_split(communicator_, odd_color_, 0, &group_communicator);
		MPI_Comm_split(communicator_, odd_color_, 0, &equivalent_ranks_comm);
		MPI_Barrier(communicator_);
		unsigned long dummy_local_value = std::numeric_limits<AdjacencyListGraph::Weight>::max(),
					  dummy_global_value;
		// Match the global reduction that happens in grouped nodes
		MPI_Reduce(&dummy_local_value, &dummy_global_value, 1, MPI_UNSIGNED_LONG, MPI_MIN, 0, MPI_COMM_WORLD);
		MPI_Reduce(&MPI::total, NULL, 1, MPI_DOUBLE, MPI_MAX, 0, communicator_);
		return {};
	}


	sitmo::prng_engine random(seed + rank_);
	int32_t seed1 = random.operator()();
	int32_t seed2 = random.operator()();

	int group_color = rank_ % group_count;
	MPI_Comm_split(communicator_, group_color, 0, &group_communicator);
	int group_rank; // Our rank within group
	MPI_Comm_rank(group_communicator, &group_rank);
	// Create communicator for slice broadcast
	MPI_Comm_split(communicator_, group_rank, 0, &equivalent_ranks_comm);

	unsigned t = intermediate_size(samplerFactory.vertex_count_, samplerFactory.edge_count_);

	std::unique_ptr<WeightedIteratedSparseSampling> sampler = samplerFactory.build(group_communicator, group_color, group_size, seed1, t);

	TimeUtils::profileStep([&]() {
		if (group_color == 0) {
			sampler->loadSlice();
		}
	}, rank_, "load_slice");

	MPI_Barrier(communicator_);
	PAPI_START();

	if (group_color == 0) {
		sampler->broadcastSlice(equivalent_ranks_comm);
	} else {
		sampler->receiveSlice(equivalent_ranks_comm);
	}

	TimeUtils::measure<void>([&]() {
		MPI::total = 0;
		TimeUtils::profileStep([&]() {
			sampler->shrink();
		}, rank_, "sampler.shrink");

		GraphSlice graph_slice = TimeUtils::profileStep<GraphSlice>([&]() {
			return sampler->reduce();
		}, rank_, "sampler.reduce");


		// FIXME: The thing with longs is very unfortunate. not how templates should be used
		// Whoever did https://github.com/glukas/parallel-minimum-cut/blob/master/src/recursive_contract.hpp#L16
		// shouldn't be allowed to touch templates ever again.
		unsigned long trial_result = TimeUtils::profileStep<unsigned long>([&]() {
			return (unsigned long) mincut::parallel_cut(
					group_communicator,
					graph_slice,
					2,
					seed2
			);
		}, rank_, "RC");

		// Hack: reduce across the world. Odd nodes supply max value
		MPI::Reduce(&trial_result, &result.weight, 1, MPI_UNSIGNED_LONG, MPI_MIN, 0, MPI_COMM_WORLD);
	}, result.cuttingTime);


	double global_mpi;
	MPI_Reduce(&MPI::total, &global_mpi, 1, MPI_DOUBLE, MPI_MAX, 0, communicator_);
	result.mpiTime = global_mpi;

	PAPI_STOP(rank_, MPI::total);

	// Only the master process returns a valid result
	return result;
}

SquareRootCut::Result SquareRootCut::runConcurrentMaster(SamplerFactory & samplerFactory, double success_probability, uint32_t seed) {
	return participateInGroup(samplerFactory, success_probability, seed);
}

void SquareRootCut::runConcurrentWorker(SamplerFactory & samplerFactory, double success_probability, uint32_t seed) {
	participateInGroup(samplerFactory, success_probability, seed);
}

SquareRootCut::Result SquareRootCut::seqMaster(GraphInputIterator & input, double success_probability, uint32_t seed) {
	SquareRootCut::Result result;
	result.variant = LOW_CONCURRENCY;

	AdjacencyListGraph graph = TimeUtils::profile<AdjacencyListGraph>([&]() {
		return AdjacencyListGraph::fromIterator(input);
	}, "load_input");

	AdjacencyListGraph::EdgeList edges = graph.edges();
	AdjacencyListGraph::Weight local_min;

	double total_trials = numberOfTrials(graph.vertex_count(), graph.edge_count(), success_probability);
	unsigned trials = (unsigned) std::ceil(total_trials);

	result.trials = trials;

	// We need references for MPI
	unsigned edge_count = graph.edge_count(),
			vertex_count = graph.vertex_count();

	PAPI_START();

	TimeUtils::measure<void>([&]() {
		sitmo::prng_engine engine(seed);

		unsigned t = intermediate_size(vertex_count, edge_count);

		for (unsigned i{0}; i < trials; i++) {
			TimeUtils::profile<void>([&]() {
				local_min = std::min(local_min, SequentialSquareRootCut(
						std::make_shared<AdjacencyListGraph>(AdjacencyListGraph(graph)).get(), &engine, t).compute());
			}, "local_trial");
		}

		result.weight = local_min;
	}, result.cuttingTime);

	PAPI_STOP(rank_, 0);

	return result;
}
