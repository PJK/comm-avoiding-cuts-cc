#include "../SquareRootCut.hpp"
#include "input/GraphInputIterator.hpp"
#include "../utils.hpp"

int main(int argc, char* argv[])
{
	if (argc != 4) {
		std::cout << "Usage: square_root PROBABILITY INPUT_FILE SEED" << std::endl;
		return 1;
	}

	float success_probability { std::stof(argv[1], nullptr) };
	uint32_t seed = { (uint32_t) std::stoi(argv[3]) };

	SquareRootCut cutter;

	GraphInputIterator input(argv[2]);

	std::cout << std::fixed;

	if (cutter.master()) {
		SquareRootCut::Result res = cutter.seqMaster(input, success_probability, seed);
		std::cout << argv[2] << ","
				  << seed << ","
				  << 1 << ","
				  << input.vertexCount() << ","
				  << input.edgeCount() << ","
				  << res.cuttingTime << ","
				  << res.mpiTime << ","
				  << res.trials << ","
				  << (res.variant == SquareRootCut::Variant::HIGH_CONCURRENCY ? "high" : "low") << ","
				  << res.weight << std::endl;
	} else {
		cutter.runWorker(input, success_probability, seed);
	}
}
