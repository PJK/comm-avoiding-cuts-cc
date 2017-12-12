#include "../ApproximateCut.hpp"
#include "input/GraphInputIterator.hpp"
#include "../utils.hpp"

int main(int argc, char* argv[])
{
	if ((argc != 4) && (argc != 5)) {
		std::cout << "Usage: slimGraph-Light PROBABILITY INPUT_FILE|CLICK [SIZE] SEED" << std::endl;
		return 1;
	}

	float success_probability { std::stof(argv[1], nullptr) };
	uint32_t seed = { (uint32_t) std::stoi(argv[argc == 4 ? 3 : 4]) };

	MPI_Init(&argc, &argv);

	ApproximateCut cutter(MPI_COMM_WORLD, seed);

	if (std::string(argv[2]) == "CLICK") {
		cutter.loadFromCLICK((unsigned) std::stoul(argv[3]));
	} else {
		GraphInputIterator input(argv[2]);
		cutter.loadFromInput(input);
	}

	std::cout << std::fixed;

	if (cutter.master()) {
		ApproximateCut::Result res = cutter.run(success_probability);
		std::cout << argv[2] << ","
				  << seed << ","
				  << cutter.processors() << ","
				  << cutter.initialVertexCount() << ","
				  << cutter.initialEdgeCount() << ","
				  << res.cuttingTime << ","
				  << res.mpiTime << ","
				  << res.trials << ","
				  << res.weight << std::endl;
	} else {
		cutter.run(success_probability);
	}

	MPI_Finalize();
}
