#include <iostream>
#include "modularity.h"

ModulWorker::ModulWorker(string nodes, string edges) : graph(nodes, edges) {
    cout << "ModulWorker initalized\n";
}

double ModulWorker::calculate_modularity(int start_node, int end_node) {
    /*
    * this function calculates modularity of a graph using 'classical' formula
    * i. e. formula that iterates over every pair of nodes
    */
    vector<int> communities(graph.num_nodes + 1, -1);
    graph.get_communities(communities);

    // modularity
    double Q = 0;

    double adj_matrix, prob;
    for (int i = start_node; i <= end_node; i++) {
        for (int j = 1; j <= graph.num_nodes; j++) {
            // skip if nodes are the same or are not in the same community
            if (communities[i] != communities[j]) continue;
            
            // check if i and j are connected
            adj_matrix = 0;
            for (auto it = graph.adj_list[i].begin(); it != graph.adj_list[i].end(); it++) {
                if (*it == j) {
                    adj_matrix = 1;
                    break;
                }
            }

            prob = (double)(graph.node_degree(i) * graph.node_degree(j));
            prob /= (double)(2 * graph.num_edges);

            Q += adj_matrix - prob;
        }
    }

    Q /= 2 * graph.num_edges;

    return Q;
}

double ModulWorker::calculate_modularity_comm(int start_node, int end_node) {
    /*
    * this function calculates modularity of a graph using formula that
    * iterates over communities of that graph 
    */
    vector<int> communities(graph.num_nodes + 1, -1);
    graph.get_communities(communities);
    
    // finding num of communities
    int num_c = 0;
    for (int c : communities) num_c = max(c, num_c);

    // algorithm
    double Q = 0;
    for (int c = 1; c <= num_c; c++) {
        double lc = 0;
        double kc = 0;

        // calculating kc
        for (int i = 1; i <= graph.num_nodes; i++) {
            if (communities[i] == c) kc += graph.node_degree(i);
        }

        // calculating lc
        for (auto it = graph.edge_ids.begin(); it != graph.edge_ids.end(); it++) {

            if (communities[it->first.first] == c && 
                    communities[it->first.first] == communities[it->first.second])
                ++lc;
        }

        Q += (lc / (double)graph.num_edges) - (kc / (double)(2 * graph.num_edges)) * (kc / (double)(2 * graph.num_edges));
    }

    return Q;
}