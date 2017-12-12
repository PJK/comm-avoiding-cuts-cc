#ifndef PARALLEL_MINIMUM_CUT_UNWEIGHTEDGRAPH_HPP
#define PARALLEL_MINIMUM_CUT_UNWEIGHTEDGRAPH_HPP

/**
 * Contains graph definitions for UISS
 *
 * Note that we cannot have inheritance in AdjacencyListGraph, since MPI-communicated DSs need to be PODs
 */
class UnweightedGraph {
public:
	struct Edge {
		unsigned from, to;

		inline bool operator<(Edge const &other) const
		{
			return (from < other.from) || ((from == other.from) && (to < other.to));
		}

		inline bool normalized() const
		{
			return from <= to;
		}

		inline void normalize()
		{
			if (!normalized()) {
				std::swap(from, to);
			}
		}

		inline bool operator==(Edge const &other) const
		{
			return (from == other.from) && (to == other.to);
		}

		inline bool operator!=(Edge const &other) const
		{
			return !this->operator==(other);
		}

		friend std::ostream &operator<<(std::ostream &out, UnweightedGraph::Edge const &edge)
		{
			out << edge.from << " --" << edge.to;
			return out;
		}
	};
};


#endif //PARALLEL_MINIMUM_CUT_UNWEIGHTEDGRAPH_HPP
