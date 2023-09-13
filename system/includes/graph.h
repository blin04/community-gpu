#ifndef GRAPH_H

#define GRAPH_H

#include <vector>
#include <map>
#include <string>

using namespace std;


class Graph {
    private:
        void dfs(int node, vector<int> &visited, int comm_id);
    public:
        int num_nodes;
        int num_edges;
        int orig_num_edges;
        map<int, string> node_labels;
        map<pair<int, int>, int> edge_ids;
        vector<vector<int>> adj_list;

        Graph (string nodes_file, string edges_file);
        void remove_edge(int node1, int node2);
        void remove_edge(int edge_id); 
        int get_edge_id(int node1, int node2);
        int node_degree(int node);
        void print_nodes();
        void print_edges();
        void get_communities(vector<int> &communities);
};

#endif