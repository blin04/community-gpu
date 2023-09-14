#include "graph.h"

class EdgeWorker {
    public:
        Graph graph;

        EdgeWorker(string nodes, string edges);
        vector<double> calculate_edge_betweenness(int start_node, int end_node);      
};