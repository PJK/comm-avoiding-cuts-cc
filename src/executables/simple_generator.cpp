#include <iostream>
#include <random>

int main(int argc, char* argv[])
{
	if (argc != 3) {
		std::cout << "Usage: s_g N DEGREE" << std::endl;
		return 1;
	}

	unsigned long n = { std::stoul(argv[1]) };
	unsigned long d = { std::stoul(argv[2]) };

	std::cout << "# nothing to see here, move on" << std::endl;
	std::cout << n << " " << n * d << std::endl;


	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(10, 200);

	for (unsigned i = 0; i < n; i++) {
		for (unsigned j = 1; j <= d; j++) {
			std::cout << i << " " << (i + j) % n << " " << dis(gen) << std::endl;
		}
	}
}
