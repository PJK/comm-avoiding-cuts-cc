#include <iostream>
#include <random>

int main(int argc, char* argv[])
{
	if (argc != 2) {
		std::cout << "Usage: cgg n" << std::endl;
		return 1;
	}

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(1, 100);

	unsigned long n = { std::stoul(argv[1]) };

	std::cout << "# CGG" << std::endl;
	std::cout << n << " " << n * (n - 1) / 2 << std::endl;

	for (unsigned long i = 0; i < n; i++) {
		for (unsigned long j = i + 1; j < n; j++) {
			std::cout << i << " " << j << " " << dis(gen) << std::endl;
		}
	}
}
