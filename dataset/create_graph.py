"""
Create graphs used for evaluation using Stochastic Block Model.
"""

import random
import networkx as nx
import matplotlib.pyplot as plt


def draw_graph(graph):
    """
    draws given graph using networkx
    """

    # assign color to nodes for drawing
    partition = nx.community.louvain_communities(graph)
    colors = [0] * (len(community_sizes) * size_of_community) 
    col = 1
    for community in partition:
        for node in community:
            colors[node] = col
        col += 1

    # draw graph for verification
    pos = nx.spring_layout(graph, k=0.5, iterations=60)
    nx.draw_networkx(graph, pos, node_color=colors)

    plt.show()


def export_graph(graph, size_of_community):
    """
    exports graph into files 'nodes' and 'edges'
    """

    # export nodes
    nodes = open("nodes", 'w')
    for node in graph:

        # calculate to which community is current node assigned to
        community = (node // size_of_community) + 1
        nodes.write(str(node + 1) + " " + str(community) + '\n')
    nodes.close()

    # export edges
    edges = open("edges", 'w')
    for edge in graph.edges:
        edges.write(str(edge[0] + 1) + " " + str(edge[1] + 1) + '\n')
    edges.close()

size_of_community = int(input("Enter how many nodes will be in a community: "))
num_of_communities = int(input("Enter how many communities will be in a graph: "))

community_sizes = [size_of_community] * num_of_communities 
probs = []

# initalize probs as a matrix of size C * C (where C is num of communities)
for i in range(len(community_sizes)):
    row = [0] * len(community_sizes)
    probs.append(row)

# fill in probabilities of edges existing between communities i and j
for i in range(len(community_sizes)):
    for j in range(i, len(community_sizes)):
        
        if i == j: 
            # same community
            probs[i][j] = 0.1

        elif i != j:
            # different communities
            probs[i][j] = 0.01
            probs[j][i] = probs[i][j]

# generate graph
graph = nx.stochastic_block_model(community_sizes, probs)

# export graph
export_graph(graph, size_of_community)
