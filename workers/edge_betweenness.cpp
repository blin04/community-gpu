#include <queue>
#include <iostream>
#include "edge_betweenness.h"

EdgeWorker::EdgeWorker(string nodes, string edges) : graph(nodes, edges) {
    std::cout << "Edge Worker initialized\n";
}

int EdgeWorker::calculate_edge_betweenness(int start_node, int end_node) {
    /*
    * this function calculates edge betweenness values for all edges in a graph
    */

    vector<bool> visited(graph.num_nodes + 1);
    vector<int> distance(graph.num_nodes + 1);
    vector<double> weight(graph.num_nodes + 1);
    vector<double> total_betweenness(graph.orig_num_edges + 1, 0);
    vector<double> betweenness(graph.orig_num_edges + 1, 0);

    for (int node = start_node; node <= end_node; node++) {
        fill(visited.begin(), visited.end(), false);
        fill(distance.begin(), distance.end(), -1);
        fill(weight.begin(), weight.end(), 0);

        // BFS
        distance[node] = 0;
        weight[node] = 1;

        queue<int> q;
        q.push(node);
        int curr;
        while(!q.empty()) {
            curr = q.front();
            q.pop();

            if (visited[curr]) continue;
            visited[curr] = true;

            for (int next : graph.adj_list[curr]) {
                if (distance[next] == -1) {
                    // distance value not set
                    distance[next] = distance[curr] + 1;
                    weight[next] = weight[curr];
                }
                else if (distance[next] == distance[curr] + 1) {
                    // dj = di + 1;
                    weight[next] += weight[curr];
                }

                q.push(next);
            }
        }

        // vector for storing values of betweenness for each edge
        fill(betweenness.begin(), betweenness.end(), 0.0);

        // find leaf nodes
        bool leaf;
        for (int leaf_node = 1; leaf_node <= graph.num_nodes; leaf_node++) {
            if (distance[leaf_node] == -1) continue;

            leaf = true;
            for (int x : graph.adj_list[leaf_node]) {
                if (distance[x] > distance[leaf_node]) leaf = false;
            }
            if (leaf) q.push(leaf_node);
        } 

        // find betweenness values
        fill(visited.begin(), visited.end(), false);
        while(!q.empty()) {
            curr = q.front();
            q.pop();

            if (visited[curr]) continue;
            visited[curr] = true;

            // making sum of edges that have already been
            // assigned a betweenness value
            double sum = 1;
            for (int neighbour : graph.adj_list[curr]) {
                if (betweenness[graph.get_edge_id(curr, neighbour)] != 0) {
                    sum += betweenness[graph.get_edge_id(curr, neighbour)];
                } 
            }

            // assigning betweenness values
            for (int neighbour : graph.adj_list[curr]) {
                if (!visited[neighbour] && distance[neighbour] == distance[curr] - 1) {
                    betweenness[graph.get_edge_id(curr, neighbour)] = (weight[neighbour] / weight[curr]) * sum;
                    q.push(neighbour);
                }
            }
        }

        /* std::cout << "-- BETWEENNESS SCORE --\n";
        for (double x : betweenness) std::cout << x << " ";
        std::cout << "\n"; */
        


        // add betweenness scores to total betweenness values
        for (int i = 1; i <= graph.orig_num_edges; i++) total_betweenness[i] += betweenness[i];
    }

    //std::cout << "-- TOTAL BETWEENNESS SCORE --\n";
    int most_central_edge = -1;
    double centrality = -1;
    for (int i = 1; i <= graph.orig_num_edges; i++) {
        //cout << total_betweenness[i] << " ";
        if (total_betweenness[i] > centrality) {
            centrality = total_betweenness[i];
            most_central_edge = i;
        }
    }
    //cout << "\n";

    return most_central_edge;
}