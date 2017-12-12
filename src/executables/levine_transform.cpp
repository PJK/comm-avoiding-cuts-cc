#include <iostream>
#include <string>

int main() {
	std::string line;
	// Ignore
	std::getline(std::cin, line);
	std::getline(std::cin, line);
	std::cout << line << std::endl;

	while(!std::cin.eof()) {
		unsigned f, t, w;
		std::cin.ignore(2);
		std::cin >> f >> t >> w;
		std::cout << f - 1 << " " << t - 1 << " " << w << std::endl;
	}
	return 0;
}
