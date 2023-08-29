#include <queue>
#include "graph.h"

class Leader {
    public:
        queue<pair<int, int>> edges_to_delete;
        Graph graph;

        Leader(string nodes, string edges);
        void find_central_edge();
        void calculate_modularity(); 
};