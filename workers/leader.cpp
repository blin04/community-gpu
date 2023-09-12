#include <iostream>
#include <string.h>
#include <zookeeper/zookeeper.h>
#include <unistd.h>

#include "leader.h"

Leader::Leader(string nodes, string edges) : graph(nodes, edges) {
    /*
    * constructor
    */
    std::cout << "Initialized leader\n";
}

void Leader::start_eb_cluster(zhandle_t *zh) {
    /*
    * gives /eb cluster mark to start working
    */
    char start_msg[] = "start";
    int r = zoo_set(zh, "/eb", start_msg, sizeof(start_msg), -1);
    if (r != ZOK) {
        cout << "ERROR (" << r << "): couldn't set /eb to start\n";
        exit(EXIT_FAILURE);
    }
}

void Leader::start_mod_cluster(zhandle_t *zh) {
    /*
    * gives /mod cluster mark to start working
    */
    char start_msg[] = "start";
    int r = zoo_set(zh, "/mod", start_msg, sizeof(start_msg), -1);
    if (r != ZOK) {
        cout << "ERROR (" << r << "): couldn't set /mod to start\n";
        exit(EXIT_FAILURE);
    }
}

bool Leader::check_if_finished(zhandle_t *zh) {
    /*
    * this function checks if /eb and /mod clusters finished  
    * their calculations
    */

    char cluster1[20] = "", cluster2[20] = "";
    int len = sizeof(cluster1);

    int r = zoo_get(zh, "/eb", 0, cluster1, &len, NULL);
    if (r != ZOK) {
        cout << "ERROR: can't get info from /eb znode\n";
        exit(EXIT_FAILURE);
    }

    r = zoo_get(zh, "/mod", 0, cluster2, &len, NULL);
    if (r != ZOK) {
        cout << "ERROR: can't get info from /mod znode\n";
        exit(EXIT_FAILURE);
    }

   // cout << cluster1 << " " << cluster2 << "\n";

    if (strcmp(cluster1, "finished") == 0 &&
        strcmp(cluster2, "finished") == 0)
        return true;
    else return false;
}

int Leader::find_central_edge(int eb_socket) {
    /*
    * this function gets the most central edge (edge with
    * highest value of edge betweenness) in a graph by communicating
    * with the edge betweenness cluster
    */

    int edge_id;
    int r = read(eb_socket, &edge_id, sizeof(int));  
    if (r < 0) {
        std::cout << "ERROR: can't read from /eb cluster \n";
        exit(EXIT_FAILURE);
    }
    return edge_id;
}

double Leader::calculate_modularity(int mod_socket) {
    /*
    * this function gets value of modularity for a graph
    * by communicating with the modularity cluster
    */

    int edge_to_delete = edges_to_delete.front();
    edges_to_delete.pop();

    int r = write(mod_socket, &edge_to_delete, sizeof(int));
    if (r < 0) {
        cout << "ERROR (" << errno << "): failed writing to mod cluster\n";
        exit(EXIT_FAILURE);
    }

    double q;
    r = read(mod_socket, &q, sizeof(double));
    if (r < 0) {
        cout << "ERROR (" << errno << "): failed reading from mod cluster\n";
        exit(EXIT_FAILURE);
    }

    return q;
}

