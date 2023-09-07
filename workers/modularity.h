#include "graph.h"

class ModulWorker {
    public:
        Graph graph;

        ModulWorker(string nodes, string edges);
        double calculate_modularity(int start_node, int end_node);
        double calculate_modularity_comm(int start_node, int end_node);
};