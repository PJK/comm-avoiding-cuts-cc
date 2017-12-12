#ifndef GALOIS_GRAPHLABEXECUTOR_H
#define GALOIS_GRAPHLABEXECUTOR_H

#include "Galois/Bag.h"

#include <boost/mpl/has_xxx.hpp>

namespace Galois {
//! Implementation of GraphLab v2/PowerGraph DSL in Galois
namespace GraphLab {

BOOST_MPL_HAS_XXX_TRAIT_DEF(tt_needs_gather_in_edges)
template<typename T>
struct needs_gather_in_edges: public has_tt_needs_gather_in_edges<T> {};

BOOST_MPL_HAS_XXX_TRAIT_DEF(tt_needs_gather_out_edges)
template<typename T>
struct needs_gather_out_edges: public has_tt_needs_gather_out_edges<T> {};

BOOST_MPL_HAS_XXX_TRAIT_DEF(tt_needs_scatter_in_edges)
template<typename T>
struct needs_scatter_in_edges: public has_tt_needs_scatter_in_edges<T> {};

BOOST_MPL_HAS_XXX_TRAIT_DEF(tt_needs_scatter_out_edges)
template<typename T>
struct needs_scatter_out_edges: public has_tt_needs_scatter_out_edges<T> {};

struct EmptyMessage {
  EmptyMessage& operator+=(const EmptyMessage&) { return *this; }
};

template<typename Graph, typename Operator> 
struct Context {
  typedef typename Graph::GraphNode GNode;
  typedef typename Operator::message_type message_type;
  typedef std::pair<GNode,message_type> WorkItem;

private:
  template<typename,typename> friend class AsyncEngine;
  template<typename,typename> friend class SyncEngine;

  typedef std::pair<int,message_type> Message;
  typedef std::deque<Message> MyMessages;
  typedef Galois::Runtime::PerPackageStorage<MyMessages> Messages;

  Galois::UserContext<WorkItem>* ctx;
  Graph* graph;
  Galois::LargeArray<int>* scoreboard;
  Galois::InsertBag<GNode>* next;
  Messages* messages;

  Context(Galois::UserContext<WorkItem>* c): ctx(c) { }

#if defined(__IBMCPP__) && __IBMCPP__ <= 1210
public:
#endif
  Context(Graph* g, Galois::LargeArray<int>* s, Galois::InsertBag<GNode>* n, Messages* m):
    graph(g), scoreboard(s), next(n), messages(m) { }

public:

  void push(GNode node, const message_type& message) {
    if (ctx) {
      ctx->push(WorkItem(node, message));
    } else {
      size_t id = graph->idFromNode(node);
      { 
        int val = (*scoreboard)[id];
        if (val == 0 && __sync_bool_compare_and_swap(&(*scoreboard)[id], 0, 1)) {
          next->push(node);
        }
      }

      if (messages) {
        MyMessages& m = *messages->getLocal();
        int val; 
        while (true) {
          val = m[id].first;
          if (val == 0 && __sync_bool_compare_and_swap(&m[id].first, 0, 1)) {
            m[id].second += message;
            m[id].first = 0;
            return;
          }
        }
      }
    }
  }
};

template<typename Graph, typename Operator>
class AsyncEngine {
  typedef typename Operator::message_type message_type;
  typedef typename Operator::gather_type gather_type;
  typedef typename Graph::GraphNode GNode;
  typedef typename Graph::in_edge_iterator in_edge_iterator;
  typedef typename Graph::edge_iterator edge_iterator;

  typedef typename Context<Graph,Operator>::WorkItem WorkItem;

  struct Initialize {
    AsyncEngine* self;
    Galois::InsertBag<WorkItem>& bag;

    Initialize(AsyncEngine* s, Galois::InsertBag<WorkItem>& b): self(s), bag(b) { }

    void operator()(GNode n) {
      bag.push(WorkItem(n, message_type()));
    }
  };

  struct Process {
    AsyncEngine* self;
    Process(AsyncEngine* s): self(s) { }

    void operator()(const WorkItem& item, Galois::UserContext<WorkItem>& ctx) {
      Operator op(self->origOp);

      GNode node = item.first;
      message_type msg = item.second;
      
      if (needs_gather_in_edges<Operator>::value || needs_scatter_in_edges<Operator>::value) {
        self->graph.in_edge_begin(node, Galois::MethodFlag::ALL);
      }

      if (needs_gather_out_edges<Operator>::value || needs_scatter_out_edges<Operator>::value) {
        self->graph.edge_begin(node, Galois::MethodFlag::ALL);
      }

      op.init(self->graph, node, msg);
      
      gather_type sum;
      if (needs_gather_in_edges<Operator>::value) {
        for (in_edge_iterator ii = self->graph.in_edge_begin(node, Galois::MethodFlag::NONE),
            ei = self->graph.in_edge_end(node, Galois::MethodFlag::NONE); ii != ei; ++ii) {
          op.gather(self->graph, node, self->graph.getInEdgeDst(ii), node, sum, self->graph.getInEdgeData(ii));
        }
      }
      if (needs_gather_out_edges<Operator>::value) {
        for (edge_iterator ii = self->graph.edge_begin(node, Galois::MethodFlag::NONE), 
            ei = self->graph.edge_end(node, Galois::MethodFlag::NONE); ii != ei; ++ii) {
          op.gather(self->graph, node, node, self->graph.getEdgeDst(ii), sum, self->graph.getEdgeData(ii));
        }
      }

      op.apply(self->graph, node, sum);

      if (!op.needsScatter(self->graph, node))
        return;

      Context<Graph,Operator> context(&ctx);

      if (needs_scatter_in_edges<Operator>::value) {
        for (in_edge_iterator ii = self->graph.in_edge_begin(node, Galois::MethodFlag::NONE),
            ei = self->graph.in_edge_end(node, Galois::MethodFlag::NONE); ii != ei; ++ii) {
          op.scatter(self->graph, node, self->graph.getInEdgeDst(ii), node, context, self->graph.getInEdgeData(ii));
        }
      }
      if (needs_scatter_out_edges<Operator>::value) {
        for (edge_iterator ii = self->graph.edge_begin(node, Galois::MethodFlag::NONE), 
            ei = self->graph.edge_end(node, Galois::MethodFlag::NONE); ii != ei; ++ii) {
          op.scatter(self->graph, node, node, self->graph.getEdgeDst(ii), context, self->graph.getEdgeData(ii));
        }
      }
    }
  };

  Graph& graph;
  Operator origOp;

public:
  AsyncEngine(Graph& g, Operator o): graph(g), origOp(o) { }

  void execute() {
    typedef typename Context<Graph,Operator>::WorkItem WorkItem;
    typedef Galois::WorkList::dChunkedFIFO<256> WL;

    Galois::InsertBag<WorkItem> bag;
    Galois::do_all_local(graph, Initialize(this, bag));
    Galois::for_each_local(bag, Process(this), Galois::wl<WL>());
  }
};

template<typename Graph, typename Operator>
class SyncEngine {
  typedef typename Operator::message_type message_type;
  typedef typename Operator::gather_type gather_type;
  typedef typename Graph::GraphNode GNode;
  typedef typename Graph::in_edge_iterator in_edge_iterator;
  typedef typename Graph::edge_iterator edge_iterator;
  static const bool NeedMessages = !std::is_same<EmptyMessage,message_type>::value;
  typedef Galois::WorkList::dChunkedFIFO<256> WL;
  typedef std::pair<int,message_type> Message;
  typedef std::deque<Message> MyMessages;
  typedef Galois::Runtime::PerPackageStorage<MyMessages> Messages;

  Graph& graph;
  Operator origOp;
  Galois::LargeArray<Operator> ops;
  Messages messages;
  Galois::LargeArray<int> scoreboard;
  Galois::InsertBag<GNode> wls[2];
  Galois::Runtime::LL::SimpleLock<true> lock;

  struct Gather {
    SyncEngine* self;
    typedef int tt_does_not_need_push;
    typedef int tt_does_not_need_aborts;

    Gather(SyncEngine* s): self(s) { }
    void operator()(GNode node, Galois::UserContext<GNode>&) {
      size_t id = self->graph.idFromNode(node);
      Operator& op = self->ops[id];
      gather_type sum;

      if (needs_gather_in_edges<Operator>::value) {
        for (in_edge_iterator ii = self->graph.in_edge_begin(node, Galois::MethodFlag::NONE),
            ei = self->graph.in_edge_end(node, Galois::MethodFlag::NONE); ii != ei; ++ii) {
          op.gather(self->graph, node, self->graph.getInEdgeDst(ii), node, sum, self->graph.getInEdgeData(ii));
        }
      }

      if (needs_gather_out_edges<Operator>::value) {
        for (edge_iterator ii = self->graph.edge_begin(node, Galois::MethodFlag::NONE), 
            ei = self->graph.edge_end(node, Galois::MethodFlag::NONE); ii != ei; ++ii) {
          op.gather(self->graph, node, node, self->graph.getEdgeDst(ii), sum, self->graph.getEdgeData(ii));
        }
      }

      op.apply(self->graph, node, sum);
    }
  };

  template<typename Container>
  struct Scatter {
    typedef int tt_does_not_need_push;
    typedef int tt_does_not_need_aborts;

    SyncEngine* self;
    Context<Graph,Operator> context;

    Scatter(SyncEngine* s, Container& next):
      self(s),
      context(&self->graph, &self->scoreboard, &next, NeedMessages ? &self->messages : 0) 
      { }

    void operator()(GNode node, Galois::UserContext<GNode>&) {
      size_t id = self->graph.idFromNode(node);

      Operator& op = self->ops[id];
      
      if (!op.needsScatter(self->graph, node))
        return;

      if (needs_scatter_in_edges<Operator>::value) {
        for (in_edge_iterator ii = self->graph.in_edge_begin(node, Galois::MethodFlag::NONE),
            ei = self->graph.in_edge_end(node, Galois::MethodFlag::NONE); ii != ei; ++ii) {
          op.scatter(self->graph, node, self->graph.getInEdgeDst(ii), node, context, self->graph.getInEdgeData(ii));
        }
      }
      if (needs_scatter_out_edges<Operator>::value) {
        for (edge_iterator ii = self->graph.edge_begin(node, Galois::MethodFlag::NONE), 
            ei = self->graph.edge_end(node, Galois::MethodFlag::NONE); ii != ei; ++ii) {
          op.scatter(self->graph, node, node, self->graph.getEdgeDst(ii), context, self->graph.getEdgeData(ii));
        }
      }
    }
  };

  template<bool IsFirst>
  struct Initialize {
    typedef int tt_does_not_need_push;
    typedef int tt_does_not_need_aborts;

    SyncEngine* self;
    Initialize(SyncEngine* s): self(s) { }

    void allocateMessages() {
      unsigned tid = Galois::Runtime::LL::getTID();
      if (!Galois::Runtime::LL::isPackageLeader(tid) || tid == 0)
        return;
      MyMessages& m = *self->messages.getLocal();
      self->lock.lock();
      m.resize(self->graph.size());
      self->lock.unlock();
    }

    message_type getMessage(size_t id) {
      message_type ret;
      if (NeedMessages) {
        for (unsigned int i = 0; i < self->messages.size(); ++i) {
          if (!Galois::Runtime::LL::isPackageLeader(i))
            continue;
          MyMessages& m = *self->messages.getRemote(i);
          if (m.empty())
            continue;
          ret += m[id].second;
          m[id] = std::make_pair(0, message_type());
          // During initialization, only messages from thread zero
          if (IsFirst)
            break;
        }
      }
      return ret;
    }

    void operator()(GNode n, Galois::UserContext<GNode>&) {
      size_t id = self->graph.idFromNode(n);
      if (IsFirst && NeedMessages) {
        allocateMessages();
      } else if (!IsFirst) {
        self->scoreboard[id] = 0;
      }

      Operator& op = self->ops[id];
      op = self->origOp;
      op.init(self->graph, n, getMessage(id));

      // Hoist as much as work as possible behind first barrier
      if (needs_gather_in_edges<Operator>::value || needs_gather_out_edges<Operator>::value)
        return;
      
      gather_type sum;
      op.apply(self->graph, n, sum);

      if (needs_scatter_in_edges<Operator>::value || needs_scatter_out_edges<Operator>::value)
        return;
    }
  };

  template<bool IsFirst,typename Container1, typename Container2>
  void executeStep(Container1& cur, Container2& next) {
    Galois::for_each_local(cur, Initialize<IsFirst>(this), Galois::wl<WL>());
    
    if (needs_gather_in_edges<Operator>::value || needs_gather_out_edges<Operator>::value) {
      Galois::for_each_local(cur, Gather(this), Galois::wl<WL>());
    }

    if (needs_scatter_in_edges<Operator>::value || needs_scatter_out_edges<Operator>::value) {
      Galois::for_each_local(cur, Scatter<Container2>(this, next), Galois::wl<WL>());
    }
  }

public:
  SyncEngine(Graph& g, Operator op): graph(g), origOp(op) {
    ops.create(graph.size());
    scoreboard.create(graph.size());
    if (NeedMessages)
      messages.getLocal()->resize(graph.size());
  }

  void signal(GNode node, const message_type& msg) {
    if (NeedMessages) {
      MyMessages& m = *messages.getLocal();
      m[graph.idFromNode(node)].second = msg;
    }
  }

  void execute() {
    Galois::Statistic rounds("GraphLabRounds");
    Galois::InsertBag<GNode>* next = &wls[0];
    Galois::InsertBag<GNode>* cur = &wls[1];

    executeStep<true>(graph, *next);
    rounds += 1;
    while (!next->empty()) {
      std::swap(cur, next);
      executeStep<false>(*cur, *next);
      rounds += 1;
      cur->clear();
    }
  }
};

}
}
#endif
