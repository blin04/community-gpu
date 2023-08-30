#include <iostream>
#include <fstream>
#include "graph.h"

Graph::Graph(string nodes_file, string edges_file) {
    num_edges = num_nodes = 0;

    /* reading nodes */
    std::ifstream nodes_input(nodes_file);

    // initalizing first vector in adj_list to be empty
    // so that 1-indexing can be used
    adj_list.push_back(vector<int>());

    int node_id; string node_label;
    while(nodes_input >> node_id >> node_label) {
        ++num_nodes;
        node_labels[node_id] = node_label;
        adj_list.push_back(vector<int>());
    }


    /* reading edges */
    std::ifstream edges_input(edges_file);

    int node1, node2;
    while(edges_input >> node1 >> node2) {
        ++num_edges;

        // assigning an id to each edge
        if (edge_ids.find({node1, node2}) == edge_ids.end()
                && edge_ids.find({node2, node1}) == edge_ids.end()) {
            edge_ids[{node1, node2}] = num_edges;
        }

        adj_list[node1].push_back(node2);
        adj_list[node2].push_back(node1);
    } 

    orig_num_edges = num_edges;
}

void Graph::remove_edge(int node1, int node2) {
    /* removes given edge */

    // removing in node1's adjacency list
    for (auto it = adj_list[node1].begin(); it != adj_list[node1].end(); it++) {
        if (*it == node2) {
            adj_list[node1].erase(it);
            break;
        }
    }

    // removing in node2's adjacency list
    for (auto it = adj_list[node2].begin(); it != adj_list[node2].end(); it++) {
        if (*it == node1) {
            adj_list[node2].erase(it);
            break;
        }
    }

    // removing corresponding id
    if (edge_ids.find({node1, node2}) == edge_ids.end()) edge_ids.erase(edge_ids.find({node2, node1}));
    else edge_ids.erase(edge_ids.find({node1, node2}));

    --num_edges;
}

int Graph::get_edge_id(int node1, int node2) {
    if (edge_ids.find({node1, node2}) == edge_ids.end()) 
        return edge_ids[{node2, node1}];
    else return edge_ids[{node1, node2}];
}

int Graph::node_degree(int node) {
    /* returns degree of a given node */
    return (int)adj_list[node].size();
}

void Graph::print_nodes() {
    /* prints out nodes of a graph */

    cout << "--- PRINTING NODES ---\n";
    for (int i = 1; i <= num_nodes; i++) {
        cout << i << " " << node_labels[i] << "\n";
    }
}

void Graph::print_edges() {
    cout << "--- PRINTING EDGES ---\n";
    for (auto it = edge_ids.begin(); it != edge_ids.end(); it++) {
        cout << "(" << it->first.first << " " << it->first.second << ") - " << it->second << "\n";
    }
}

// to do...
void Graph::get_communities() {}