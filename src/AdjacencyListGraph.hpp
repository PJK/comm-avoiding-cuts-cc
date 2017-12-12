#ifndef PARALLEL_MINIMUM_CUT_ADJACENCYLISTGRAPH_HPP
#define PARALLEL_MINIMUM_CUT_ADJACENCYLISTGRAPH_HPP

#include <vector>
#include <tuple>
#include "DisjointSets.hpp"
#include "prng_engine.hpp"
#include "UnweightedGraph.hpp"


class AdjacencyListGraph {
public:
	typedef unsigned long Weight;
	struct Edge {
		unsigned from, to;
		Weight weight;

		inline bool operator<(Edge const & other) const {
			return (from < other.from) || ((from == other.from) && (to < other.to));
		}

		inline bool normalized() const {
			return from <= to;
		}

		inline void normalize() {
			if (! normalized()) {
				std::swap(from, to);
			}
		}

		inline bool operator==(Edge const & other) const {
			return (from == other.from) && (to == other.to);
		}

		inline bool operator!=(Edge const & other) const {
			return ! this->operator==(other);
		}

		inline UnweightedGraph::Edge dropWeight() const {
			return { from, to };
		}

		friend std::ostream & operator<< (std::ostream & out, AdjacencyListGraph::Edge const & edge) {
			out << edge.from << " --" << edge.weight << "-- " << edge.to;
			return out;
		}
	};

	static_assert(sizeof(Edge) == 16, "Expecting 4B unsigneds and 16B ulongs (else we are wasting space)");

	typedef std::vector<Edge> EdgeList;
	static const Weight WeightUpperBound { std::numeric_limits<Weight>::max() };

private:
	EdgeList edges_;
	unsigned vertex_count_;
	DisjointSets<unsigned> disjoint_sets_;
	// This is hairy. We keep this const reference to a immutable list of edges and use it up until we do a finalizeContractionPhase(),
	// then we build our internal representation (basically CoW optimization) and switch the reference to point to it.
	EdgeList const * parent_edges_;
public:
	AdjacencyListGraph(unsigned vertex_count) : vertex_count_(vertex_count), disjoint_sets_(vertex_count + 1), parent_edges_(&edges_)
	{}

	AdjacencyListGraph(const AdjacencyListGraph& that) : AdjacencyListGraph(that.vertex_count_, that.edges_)
	{}

	/**
	 * Edge source has to implement:
	 * - unsigned vertexCount()
	 * - InputIterator-compatible iterator type Iterator over AdjacencyListGraph::Edge
	 * - Iterator begin()
	 * - Iterator end()
	 */
	template<class EdgeSource>
	static AdjacencyListGraph fromIterator(EdgeSource & source) {
		AdjacencyListGraph graph(source.vertexCount());
		for (auto e : source)
			graph.addEdge(e);
		return graph;
	}

	static AdjacencyListGraph fromAdjacencyMatrix(const std::vector<Weight> & matrix, unsigned vertex_count) {
		AdjacencyListGraph graph(vertex_count);

		for (unsigned i { 0 }; i < vertex_count; i++) {
			for (unsigned j { 0 }; j <= i; j++) {
				Weight weight = matrix.at(i * vertex_count + j);
				if (weight > 0u) {
					graph.addEdge(i, j, matrix.at(i * vertex_count + j));
				}
				assert(matrix.at(i * vertex_count + j) == matrix.at(j * vertex_count + i));
			}
		}

		return graph;
	}

	// Handy when passing edge arrays over MPI
	AdjacencyListGraph(unsigned vertex_count, const EdgeList & edges) : edges_(edges), vertex_count_(vertex_count), disjoint_sets_(vertex_count + 1), parent_edges_(&edges_)
	{}

	AdjacencyListGraph(unsigned vertex_count, const EdgeList & edges, bool _use_parent_edges) :  vertex_count_(vertex_count), disjoint_sets_(vertex_count + 1), parent_edges_(&edges)
	{}

	AdjacencyListGraph(const AdjacencyListGraph& that, bool _use_parent_edges) : AdjacencyListGraph(that.vertex_count_, that.edges_, _use_parent_edges)
	{}

	void addEdge(unsigned from, unsigned to, Weight weight);

	void addEdge(Edge e);

	/**
	 * return The graph with singleton vertices removed and edges renamed so that it is a connected graph on [0, vertex_count)
	 */
	std::unique_ptr<AdjacencyListGraph> compact() const;

	/**
	 * Contract the edge `from -- to`
	 * If such an edge does not exist, no action is taken
	 * Loop are not removed and parallel edges are not merged after the contraction
	 * The edge count will not be accurate while the representation is denormalized
	 */
	void weaklyContractEdge(unsigned from, unsigned to);

	/**
	 * Contraction with loop removal and edge merging
	 */
	void contractEdge(unsigned from, unsigned to);

	/**
	 * Remove loop and merge edges after a series of weaklyContractEdge() calls
	 */
	void finalizeContractionPhase(sitmo::prng_engine * random);

	unsigned vertex_count() const
	{
		return vertex_count_;
	}

	unsigned edge_count() const
	{
		return unsigned(parent_edges_->size());
	}

	unsigned maxVertexID() const;

	EdgeList const & edges() const;

	friend std::ostream & operator<< (std::ostream & out, AdjacencyListGraph const & graph);

private:
	std::tuple<unsigned, unsigned> normalize(unsigned v1, unsigned v2) const;
};



#endif //PARALLEL_MINIMUM_CUT_ADJACENCYLISTGRAPH_HPP
