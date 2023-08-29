#include <queue>
#include <iostream>
#include "edge_betweenness.h"

EdgeWorker::EdgeWorker(string nodes, string edges) : graph(nodes, edges) {
    cout << "Edge Worker initialized\n";
}

int EdgeWorker::calculate_edge_betweenness(int start_node, int end_node) {
    /*
    * this function calculates edge betweenness values for all edges in a graph
    */

    vector<bool> visited(graph.num_nodes + 1);
    vector<int> distance(graph.num_nodes + 1);
    vector<int> weight(graph.num_nodes + 1);

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

            if (node == 1) cout << "Curr: " << curr << "\n";

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

        if (node == 1) {
            cout << "--- DISTANCE ---\n";
            for (auto d : distance) cout << d << " ";
            cout << "\n";

            cout << "--- WEIGHT ---\n";
            for (auto d : weight) cout << d << " ";
            cout << "\n";
        }
    }

    return 0;
}