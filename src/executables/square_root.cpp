#include "../SquareRootCut.hpp"
#include "input/GraphInputIterator.hpp"
#include "../utils.hpp"

int main(int argc, char* argv[])
{
	if ((argc != 4) && (argc != 5)) {
		std::cout << "Usage: square_root PROBABILITY INPUT_FILE|CLICK [SIZE] SEED" << std::endl;
		return 1;
	}

	float success_probability { std::stof(argv[1], nullptr) };
	uint32_t seed = { (uint32_t) std::stoi(argv[argc == 4 ? 3 : 4]) };

	MPI_Init(&argc, &argv);

	SquareRootCut cutter(MPI_COMM_WORLD);

	std::cout << std::fixed;

	if (cutter.master()) {
		SquareRootCut::Result res;

		if (std::string(argv[2]) == "CLICK") {
			res = cutter.runClickMaster((unsigned) std::stoul(argv[3]), success_probability, seed);
		} else {
			GraphInputIterator input(argv[2]);
			res = cutter.runMaster(input, success_probability, seed);
		}

		std::cout << argv[2] << ","
				  << seed << ","
				  << cutter.processors() << ","
				  << cutter.initialVertexCount() << ","
				  << cutter.initialEdgeCount() << ","
				  << res.cuttingTime << ","
				  << res.mpiTime << ","
				  << res.trials << ","
				  << (res.variant == SquareRootCut::Variant::HIGH_CONCURRENCY ? "high" : "low") << ","
				  << res.weight << std::endl;
	} else {
		if (std::string(argv[2]) == "CLICK") {
			cutter.runClickWorker((unsigned) std::stoul(argv[3]), success_probability, seed);
		} else {
			GraphInputIterator input(argv[2]);
			cutter.runWorker(input, success_probability, seed);
		}
	}

	MPI_Finalize();
}
