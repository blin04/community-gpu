#include <vector>
#include <map>
#include <string>

using namespace std;

class Graph {
    public:
        int num_nodes;
        int num_edges;
        map<int, string> node_labels;
        vector<vector<int>> adj_list;

        Graph (string nodes_file, string edges_file);
        void remove_edge(int node1, int node2);
        int node_degree(int node);
        void print_nodes();
        void print_edges();
        void get_communities();
};