#ifndef PARALLEL_MINIMUM_CUT_MPICOLLECTOR_HPP
#define PARALLEL_MINIMUM_CUT_MPICOLLECTOR_HPP

#include <mpi.h>
#include "utils.hpp"

/**
 * Stubs out MPI operations with profiled ones. After this, I'm headed to industry.
 */

// Do you believe in magic?
#define MPI_WRAP(FUNCTION) template<typename... Args> \
	int FUNCTION (Args&&... args) { \
	return wrap<int>(MPI_ ## FUNCTION, std::forward<Args>(args)...); \
	}

namespace MPI {
	extern double total;

	template<typename T, typename F, typename... Args>
	T wrap(F f, Args&&... args)
	{
		double timer;
		T result = TimeUtils::measure<T>([&]() {
			return f(std::forward<Args>(args)...);
		}, timer);

		total += timer;

		// NRVO
		return result;
	}

	MPI_WRAP(Bcast);
	MPI_WRAP(Reduce);
	MPI_WRAP(Gather);
	MPI_WRAP(Gatherv);
	MPI_WRAP(Scatter);
	MPI_WRAP(Allgather);
	MPI_WRAP(Allgatherv);
	MPI_WRAP(Alltoall);
	MPI_WRAP(Alltoallv);
	MPI_WRAP(Allreduce);
	MPI_WRAP(Barrier);
	MPI_WRAP(Isend);
	MPI_WRAP(Irecv);
	MPI_WRAP(Wait);
	MPI_WRAP(Type_vector);
	MPI_WRAP(Type_commit);
	MPI_WRAP(Ibsend);
	MPI_WRAP(Type_free);
}

#endif //PARALLEL_MINIMUM_CUT_MPICOLLECTOR_HPP
