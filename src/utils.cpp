#include "utils.hpp"
#include <numeric>
#include <algorithm>

std::vector<int> MPIUtils::prefix_offsets(const std::vector<int> & sizes) {
	std::vector<int> offsets;
	offsets.push_back(0);
	std::partial_sum(
			sizes.begin(),
			--sizes.end(),
			std::back_inserter(offsets)
	);
	return offsets; // NRVO
}

std::vector<int> MPIUtils::prefix_intervals(const std::vector<int> & sizes) {
	std::vector<int> offsets;
	offsets.push_back(0);
	std::partial_sum(
			sizes.begin(),
			sizes.end(),
			std::back_inserter(offsets)
	);
	return offsets; // NRVO
}


void CacheUtils::trashCache(unsigned cache_size) {
	// Trashe le cache
	std::vector<uint64_t> vec(cache_size * 1000000 / 8);
	std::iota(vec.begin(), vec.end(), 0);
	std::random_shuffle(vec.begin(), vec.end());

	// Pointer chase
	size_t loc = vec.at(0);
	while (loc != 0) {
		loc = vec.at(loc);
	}
}
