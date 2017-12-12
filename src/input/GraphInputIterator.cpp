#include "GraphInputIterator.hpp"

void GraphInputIterator::open()
{
	read_ = 0;
	file_.open(name_, std::ios::in);
	// TODO allow multiline comments
	file_.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	file_ >> vertices_;
	file_ >> lines_;
}

GraphInputIterator::Iterator GraphInputIterator::begin()
{
	return Iterator(false, *this, read());
}

GraphInputIterator::Iterator GraphInputIterator::end()
{
	return Iterator(true, *this, { 0, 0, 0 });
}

void GraphInputIterator::reopen()
{
	file_.close();
	open();
}

AdjacencyListGraph::Edge GraphInputIterator::read()
{
	unsigned f, t, w;
	file_ >> f >> t >> w;
	read_++;
//	assert (f != t);
	return { f , t , w }; // Iterators FTW
}
