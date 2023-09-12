#include <queue>

#include "graph.h"

class Leader {
    public:
        queue<int> edges_to_delete;
        vector<pair<int, int>> removal_order;
        vector<double> modularity_values;
        Graph graph;

        Leader(string nodes, string edges);
        void start_eb_cluster(zhandle_t* zh);
        void start_mod_cluster(zhandle_t* zh);
        bool check_if_finished(zhandle_t* zh);
        int find_central_edge(int eb_socket);
        double calculate_modularity(int mod_socket); 
        void get_best_partition();
};