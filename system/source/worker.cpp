#include <iostream>
#include <fstream>
#include <string>

#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <zookeeper/zookeeper.h>

#include "../includes/edge_betweenness.h"
#include "../includes/modularity.h"
#include "../includes/globals.h"

using namespace std;

string HOSTNAME = "";
string MY_IP = "";
string LEADER_IP = "";
int LEADER_PORT = 11000;

void setHostName() {
    /*
    * function for setting hostname of machine 
    * where program is being executed 
    */

    char buff[100];
    int hostname = gethostname(buff, sizeof(buff));

    HOSTNAME = buff;

    if (hostname == -1) {
        cout << "ERROR: failed in setHostName()\n";
        exit(EXIT_FAILURE);
    }
}

void setIPAdress() {
    /*
    * function for setting IP address of machine
    * where program is being executed 
    */

    struct hostent* host_entry = gethostbyname(&HOSTNAME[0]);

    if (host_entry == NULL) {
        cout << "ERROR: failed in getIPAdress()\n";
        exit(EXIT_FAILURE);
    }

    char* IPbuffer = inet_ntoa(*(struct in_addr*)
                                    host_entry->h_addr_list[0]);

    MY_IP = IPbuffer;
}

bool checkCluster(zhandle_t* zh, string path) {
    /* 
    * checks if machine is assigned to some particular
    * cluster indicated by path variable
    */  

    struct String_vector children;
    int r = zoo_get_children(zh, &path[0], 0, &children);
    if (r != ZOK) {
        cout << "ERROR: failed in checkCluster()\n";
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < children.count; i++) {
        if (HOSTNAME == children.data[i]) return true;
    }
    return false;
}

int main()
{
    //zoo_set_debug_level(ZOO_LOG_LEVEL_DEBUG);

    // initiating zookeeper connection
    zhandle_t *zkHandler = zookeeper_init("localhost:2181", NULL, 1000, 0, 0, 0);

    if (zkHandler == NULL) {
        cout << "Failed to connect to Zookeeper\n";
        exit(EXIT_FAILURE);
    }
    cout << "Connected sucessfully!\n";

    setHostName();
    setIPAdress();

    if (checkCluster(zkHandler, "/eb")) {
        // this worker is assigned to eb cluster

        // get IP address of cluster leader
        int len = sizeof(LEADER_IP);
        char ip_buff[len] = "";
        string path_to_leader = "/eb/" + EB_NODES[0];
        int r = zoo_get(zkHandler, &path_to_leader[0], 0, ip_buff, &len, NULL);
        if (r != ZOK) {
            cout << "ERROR (" << r << "): can't get leader ip \n";
            exit(EXIT_FAILURE);
        }
        LEADER_IP = ip_buff;
        cout << "Cluster Leader IP: " << LEADER_IP << "\n";

        // open socket for communication with cluster leader
        int leader_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (leader_socket < 0) {
            cout << "ERROR (" << errno << "): can't create socket\n";
            exit(EXIT_FAILURE);
        }

        // struct containing address of cluster leader
        sockaddr_in leader_addr;
        leader_addr.sin_family = AF_INET;
        leader_addr.sin_port = htons(LEADER_PORT);

        // convert IPv4 address of leader
        // from text to binary form
        r = inet_pton(AF_INET, &LEADER_IP[0], &leader_addr.sin_addr);
        if (r <= 0) {
            cout << "ERROR (" << r << "): invalid address\n";
            exit(EXIT_FAILURE);
        } 

        // connect to cluster leader
        r = connect(leader_socket, (sockaddr*)&leader_addr, sizeof(leader_addr));
        if (r < 0) {
            cout << "ERROR (" << strerror(errno) << "): can't connect\n";
        }
        cout << "Connection with cluster leader sucessful!\n";

        // calculate edge betweenness
        EdgeWorker worker(NODES_PATH, EDGES_PATH);

        int start_node, end_node, edge_to_delete = 1;
        vector<double> betweenness(worker.graph.orig_num_edges + 1, 0);
        while(worker.graph.num_edges) {
            // read interval from cluster leader
            cout << "waiting to read...";
            r = read(leader_socket, &start_node, sizeof(start_node));
            if (r < 0) {
                cout << "ERROR (" << errno << "): failed reading starting node\n";
                exit(EXIT_FAILURE);
            }

            r = read(leader_socket, &end_node, sizeof(end_node));
            if (r < 0) {
                cout << "ERROR (" << errno << "): failed reading ending node\n";
                exit(EXIT_FAILURE);
            }

            betweenness = worker.calculate_edge_betweenness(start_node, end_node);

            // send results to leader
            for (int i = 1; i <= worker.graph.orig_num_edges; i++) {
                r = write(leader_socket, &betweenness[i], sizeof(betweenness[i]));
                if (r < 0) {
                    cout << "ERROR (" << errno << "): failed sending results to leader\n";
                    exit(EXIT_FAILURE);
                }
            }

            // read edge that should be deleted
            r = read(leader_socket, &edge_to_delete, sizeof(edge_to_delete));
            if (r < 0) {
                cout << "ERROR (" << errno << "): failed reading edge to delete from leader\n";
                exit(EXIT_FAILURE);
            }
            worker.graph.remove_edge(edge_to_delete);
        }
    }
    else if (checkCluster(zkHandler, "/mod")) {
        // this worker is assigned to mod cluster

        // get IP address of cluster leader
        int len = sizeof(LEADER_IP);
        char ip_buff[len] = "";
        string path_to_leader = "/mod/" + MOD_NODES[0];
        int r = zoo_get(zkHandler, &path_to_leader[0], 0, ip_buff, &len, NULL);
        if (r != ZOK) {
            cout << "ERROR (" << r << "): can't get leader ip \n";
            exit(EXIT_FAILURE);
        }
        LEADER_IP = ip_buff;
        cout << "Cluster Leader IP: " << LEADER_IP << "\n";

        // open socket for communication with cluster leader
        int leader_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (leader_socket < 0) {
            cout << "ERROR (" << errno << "): can't create socket\n";
            exit(EXIT_FAILURE);
        }

        // struct containing address of cluster leader
        sockaddr_in leader_addr;
        leader_addr.sin_family = AF_INET;
        leader_addr.sin_port = htons(LEADER_PORT);

        // convert IPv4 address of leader
        // from text to binary form
        r = inet_pton(AF_INET, &LEADER_IP[0], &leader_addr.sin_addr);
        if (r <= 0) {
            cout << "ERROR (" << r << "): invalid address\n";
            exit(EXIT_FAILURE);
        } 

        // connect to cluster leader
        r = connect(leader_socket, (sockaddr*)&leader_addr, sizeof(leader_addr));
        if (r < 0) {
            cout << "ERROR (" << strerror(errno) << "): can't connect\n";
        }
        cout << "Connection with cluster leader sucessful!\n";

        // main calculation of modularity
        ModulWorker worker(NODES_PATH, EDGES_PATH);

        vector<int> comm(worker.graph.num_nodes + 1);
        int start_node, end_node;
        double q;
        while(1) {
            // read interval from cluster leader
            r = read(leader_socket, &start_node, sizeof(start_node));
            if (r < 0) {
                cout << "ERROR (" << errno << "): failed reading starting node\n";
                exit(EXIT_FAILURE);
            }

            // signal from leader to stop calculating
            if (start_node == -1) break;

            r = read(leader_socket, &end_node, sizeof(end_node));
            if (r < 0) {
                cout << "ERROR (" << errno << "): failed reading ending node\n";
                exit(EXIT_FAILURE);
            }

            // read communities from cluster leader
            r = read(leader_socket, comm.data(), comm.size() * sizeof(comm[0]));
            if (r < 0) {
                cout << "ERROR (" << errno << "): failed reading communities from leader\n";
                exit(EXIT_FAILURE);
            }

            // calculate modularity
            q = worker.calculate_modularity(start_node, end_node, comm);

            r = write(leader_socket, &q, sizeof(q));
            if (r < 0) {
                cout << "ERROR (" << errno << "): failed writing result to leader\n";
                exit(EXIT_FAILURE);
            }
        }
    }
    else {
        cout << "ERROR: worker isn't assigned to any cluster!\n";
        exit(EXIT_FAILURE);
    }


    return 0;
}