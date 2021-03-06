/* @file
 * @section License
 *
 * Galois, a framework to exploit amorphous data-parallelism in irregular
 * programs.
 *
 * Copyright (C) 2011, The University of Texas at Austin. All rights reserved.
 * UNIVERSITY EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES CONCERNING THIS
 * SOFTWARE AND DOCUMENTATION, INCLUDING ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR ANY PARTICULAR PURPOSE, NON-INFRINGEMENT AND WARRANTIES OF
 * PERFORMANCE, AND ANY WARRANTY THAT MIGHT OTHERWISE ARISE FROM COURSE OF
 * DEALING OR USAGE OF TRADE.  NO WARRANTY IS EITHER EXPRESS OR IMPLIED WITH
 * RESPECT TO THE USE OF THE SOFTWARE OR DOCUMENTATION. Under no circumstances
 * shall University be liable for incidental, special, indirect, direct or
 * consequential damages or loss of profits, interruption of business, or
 * related expenses which may arise from use of Software or Documentation,
 * including but not limited to those resulting from defects in Software and/or
 * Documentation, or loss or inaccuracy of data of any kind.
 *
 * @author Nikunj Yadav nikunj@cs.utexas.edu
 */

#ifndef GRAPHREADER_H_
#define GRAPHREADER_H_
#include <fstream>
#include <vector>
using namespace std;

typedef Galois::Graph::LC_CSR_Graph<int, unsigned int> InputGraph;
typedef Galois::Graph::LC_CSR_Graph<int, unsigned int>::GraphNode InputGNode;



    while (true) {
      int index = strtol(items, &remaining,10) - 1;
      if(index < 0) break;
      items = remaining;
      GNode n2 = nodes[index];
      if(n1==n2){
        continue;
      }
      graph->addEdge(n1, n2, Galois::MethodFlag::ALL, 1);
      graph->getData(n1).setEdgeWeight(graph->getData(n1).getEdgeWeight() + 1);
      graph->getData(n1).setNumEdges(graph->getData(n1).getNumEdges() + 1);
      countEdges++;
    }
  }



parallelMakeNodes(GGraph *g,vector <GNode> &gn,InputGraph *in,Galois::GAccumulator<int> &numNodes):
  graph(g),inputGraph(in),gnodes(gn),pnumNodes(numNodes) {}
  void operator()(InputGNode node,Galois::UserContext<InputGNode> &ctx) {
    int id = inputGraph->getData(node);
    GNode item = graph->createNode(100,1); // FIXME: edge num
    //    graph->addNode(item);
    gnodes[id]=item;
    pnumNodes+=1;
  }
};

struct parallelMakeEdges {
  GGraph *graph;
  InputGraph *inputGraph;
  vector <GNode>  &gnodes;
  bool weighted;
  bool directed;
  Galois::GAccumulator<int> &pnumEdges;
  
  parallelMakeEdges(GGraph *g,vector <GNode> &gn,InputGraph *in,Galois::GAccumulator<int> &numE,bool w=false,bool dir = true)
    :graph(g),inputGraph(in),gnodes(gn),pnumEdges(numE) {
  weighted = w;
  directed = dir;
}

  void operator()(InputGNode inNode,Galois::UserContext<InputGNode> &ctx) {
    int nodeId = inputGraph->getData(inNode);
    GNode node = gnodes[nodeId];
    MetisNode& nodeData = graph->getData(node);
    for (InputGraph::edge_iterator jj = inputGraph->edge_begin(inNode), eejj = inputGraph->edge_end(inNode); jj != eejj; ++jj) {
      InputGNode inNeighbor = inputGraph->getEdgeDst(jj);
      if(inNode == inNeighbor) continue;
      int neighId = inputGraph->getData(inNeighbor);
      int weight = 1;
      if(weighted){
        weight = inputGraph->getEdgeData(jj);
      }
      graph->addEdge(node, gnodes[neighId], Galois::MethodFlag::ALL, weight);
      nodeData.setNumEdges(nodeData.getNumEdges() + 1);
      nodeData.setEdgeWeight(nodeData.getEdgeWeight() + weight);
      /*if(!directed){
        graph->getEdgeData(graph->addEdge(node, gnodes[neighId])) = weight;//
        nodeData.incNumEdges();
        nodeData.addEdgeWeight(weight);
        }else{
        graph->getEdgeData(graph->addEdge(node, gnodes[neighId])) = weight;
        graph->getEdgeData(graph->addEdge(gnodes[neighId], node)) = weight;
        }*/
      pnumEdges+=1;
    }

  }
};

void readGraph(MetisGraph* metisGraph, const char* filename, bool weighted = false, bool directed = true){
  InputGraph inputGraph;
  Galois::Graph::readGraph(inputGraph, filename);
  cout<<"start to transfer data to GGraph"<<endl;
  int id = 0;
  for (InputGraph::iterator ii = inputGraph.begin(), ee = inputGraph.end(); ii != ee; ++ii) {
    InputGNode node = *ii;
    inputGraph.getData(node)=id++;
  }

  GGraph* graph = metisGraph->getGraph();
  vector<GNode> gnodes(inputGraph.size());
  id = 0;
  /*for(uint64_t i=0;i<inputGraph.size();i++){
    GNode node = graph->createNode(MetisNode(id, 1));
    graph->addNode(node);
    gnodes[id++] = node;
    }*/


  typedef Galois::WorkList::dChunkedFIFO<256> WL;
  Galois::GAccumulator<int> pnumNodes;
  Galois::GAccumulator<int> pnumEdges;


  Galois::Timer t;
  t.start();
  Galois::for_each<WL>(inputGraph.begin(),inputGraph.end(),parallelMakeNodes(graph,gnodes,&inputGraph,pnumNodes),"NodesLoad");
  t.stop();
  cout<<t.get()<<" ms "<<endl;
  t.start();
  Galois::for_each<WL>(inputGraph.begin(),inputGraph.end(),parallelMakeEdges(graph,gnodes,&inputGraph,pnumEdges,weighted,true),"EdgesLoad");
  t.stop();
  cout<<t.get()<<" ms "<<endl;

  /*

    Galois::Timer t;
    t.start();
    Galois::for_each_local<WL>(inputGraph,parallelMakeNodes(graph,gnodes,&inputGraph,pnumNodes),"NodesLoad");
    t.stop();
    cout<<t.get()<<" ms "<<endl;
    t.start();
    Galois::for_each_local<WL>(inputGraph,parallelMakeEdges(graph,gnodes,&inputGraph,pnumEdges,weighted,true),"EdgesLoad");
    t.stop();
    cout<<t.get()<<" ms "<<endl;

  */

  int numNodes = pnumNodes.reduce();
  int numEdges = pnumEdges.reduce();

  // metisGraph->setNumNodes(numNodes);
  // metisGraph->setNumEdges(numEdges/2);

  cout<<"Done Reading Graph ";
  cout<<"numNodes: "<<numNodes<<"|numEdges: "<< numEdges/2 <<endl;

}




#endif /* GRAPHREADER_H_ */
