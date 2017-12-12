#ifndef PARALLEL_MINIMUM_CUT_DISJOINT_SETS_HPP
#define PARALLEL_MINIMUM_CUT_DISJOINT_SETS_HPP

#include <boost/pending/disjoint_sets.hpp>
#include <vector>

template <class ElementT>
class DisjointSets {
	std::vector<unsigned> ranks;
	std::vector<ElementT> parents;
	boost::disjoint_sets<unsigned *, ElementT *> dsets;

	std::vector<ElementT> generateParents(size_t element_count) const
	{
		std::vector<ElementT> elements(element_count);
		ElementT n = {0};
		std::generate(elements.begin(), elements.end(), [&n] { return n++; });
		return elements; // NRVO
	}

public:
	DisjointSets(std::vector<ElementT> const & elements) :
			ranks(elements.size(), 0),
			parents(elements), // Every elements is its own parent (singular partitions)
			dsets(&ranks.at(0), &parents.at(0))
	{ }

	DisjointSets(size_t element_count) : DisjointSets(generateParents(element_count))
	{ }

	DisjointSets(const DisjointSets& that) = delete;

	ElementT find(ElementT elem) /* const */
	{
		return dsets.find_set(elem);
	}

	void unify(ElementT a, ElementT b)
	{
		dsets.link(a, b);
	}
};


#endif //PARALLEL_MINIMUM_CUT_DISJOINT_SETS_HPP
