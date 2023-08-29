#include <queue>
#include "graph.h"

class Leader {
    public:
        queue<pair<int, int>> edges_to_delete;
        vector<pair<int, pair<int, int>>> removal_order;
        Graph graph;

        Leader(string nodes, string edges);
        void find_central_edge();
        void calculate_modularity(); 
        void get_best_partition();
};