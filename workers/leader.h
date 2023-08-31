#include <queue>
#include "graph.h"

class Leader {
    public:
        queue<pair<int, int>> edges_to_delete;
        vector<pair<int, pair<int, int>>> removal_order;
        Graph graph;

        Leader(string nodes, string edges);
        void start_eb_cluster(zhandle_t* zh);
        void start_mod_cluster(zhandle_t* zh);
        void find_central_edge();
        void calculate_modularity(); 
        void get_best_partition();
};