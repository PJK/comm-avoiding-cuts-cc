import networkx as nx
import os
from random import randint

def set_weights(G, weight):
    for edge in G.edges_iter():
        G[edge[0]][edge[1]]['weight'] = weight
    return G

def randomize_weights(G, max):
    for edge in G.edges_iter():
        G[edge[0]][edge[1]]['weight'] = randint(1, max)
    return G

def print_graph(G):
    print('{0} {1}'.format(nx.number_of_nodes(G), nx.number_of_edges(G)))
    for edge in G.edges_iter():
        print('{0} {1} {2}'.format(edge[0], edge[1], G[edge[0]][edge[1]]['weight']))

def read_stdin():
    first_line = input()
    while first_line.startswith('#'):
        first_line = input()
    n, m = map(int, first_line .split())
    G = nx.Graph()
    G.add_nodes_from(range(0, n))
    for _i in range(0, m):
        u, v, w = map(int, input().split())
        G.add_edge(u, v, weight=w)
    return G
