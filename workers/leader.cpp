#include <iostream>
#include <zookeeper/zookeeper.h>
#include "leader.h"

// konstruktor
Leader::Leader(string nodes, string edges) : graph(nodes, edges) {
    std::cout << "Initialized leader\n";
}
void Leader::find_central_edge() {
    /*
    * this function gets the most central edge (edge with
    * highest value of edge betweenness) in a graph by communicating
    * with the edge betweenness cluster
    */
    return;
}

void Leader::calculate_modularity() {
    /*
    * this function gets value of modularity for a graph
    * by communicating with the modularity cluster
    */
    return;
}

