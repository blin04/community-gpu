"""
program used for testing performances of dist algorithm
"""

import networkx as nx
from networkx.algorithms.community.centrality import girvan_newman

graph = nx.Graph()
full_graph = nx.Graph()

# read nodes
f = open("../dataset/t_n", 'r')

for line in f:
    line = line.split(' ')
    graph.add_node(int(line[0]))
    full_graph.add_node(int(line[0]))

f.close()

# read edges
f = open("../dataset/t_e", 'r')

for line in f:
    line = line.split(' ')
    graph.add_edge(int(line[0]), int(line[1]))
    full_graph.add_edge(int(line[0]), int(line[1]))

f.close()

# graph = nx.karate_club_graph()
"""
communities = girvan_newman(graph)
node_groups = []
for com in next(communities):
    node_groups.append(list(com))

print(node_groups)
"""
# calculate edge betweenness
iteration = 0
while graph.number_of_edges() > 0:
    print("---- ITERATION " + str(iteration) + "---")
    print(full_graph.number_of_edges())

    betweenness = nx.edge_betweenness_centrality(graph)
    edge_to_del = max(betweenness, key=betweenness.get)
    graph.remove_edge(*edge_to_del)

    if graph.number_of_edges() == 0:
        break

    comp = list(nx.connected_components(graph))
    mod = nx.community.modularity(full_graph, comp)

    print("Modularity is: " + str(mod))
    print("Edge to delete: " + str(edge_to_del))

    communities = sorted(comp)
    print(communities)

    iteration += 1