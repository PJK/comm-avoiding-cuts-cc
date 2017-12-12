// Computes algorithm version transition and trial distribution statistics for
// the given input parameters

#include "mpi.h"
#include "../SquareRootCut.hpp"
#include <iostream>
#include <iomanip>

int main(int argc, char* argv[])
{
	if (argc < 4) {
		std::cout << "Usage: transition N M PROBABILITY [GRANULARITY=16] [MAXP=1025]" << std::endl;
		return 1;
	}

	MPI_Init(&argc, &argv);

	unsigned long n = { std::stoul(argv[1]) };
	unsigned long m = { std::stoul(argv[2]) };
	float success_probability { std::stof(argv[3], nullptr) };
	unsigned long granularity = argc > 4 ? std::stoul(argv[4]) : 16ul;
	unsigned long maxp = argc > 5 ? std::stoul(argv[5]) : 1024ul;


	SquareRootCut cutter(MPI_COMM_WORLD);

	std::cout << "Processors  " << "Total trials  " << "Trials per CPU" << std::endl;
	for (unsigned processors = 1; processors <= maxp; processors += granularity) {
		unsigned trials = cutter.numberOfTrials(n, m, success_probability);
		unsigned tpcpu = (unsigned) std::ceil(double(trials) / processors);
		std::cout << std::setw(10) << processors
				  << std::setw(10) << trials
				  << std::setw(10) << tpcpu;

		if (processors >= SquareRootCut::group_size_ * trials) {
			int processors_per_trial = processors / trials;
			int group_size = (int) std::pow(
					2,
					std::floor(std::log2(processors_per_trial))
			);

			std::cout << "* " << processors_per_trial << ", groups size " << group_size;
		}

		std::cout << std::endl;
	}
}
