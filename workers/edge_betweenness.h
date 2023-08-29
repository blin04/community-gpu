#include "graph.h"

class EdgeWorker {
    public:
        Graph graph;

        EdgeWorker(string nodes, string edges);
        int calculate_edge_betweenness(int start_node, int end_node);      
        void remove_edge(int edge_id);
};