#include <stdio.h>
#include <iostream>
#include <string>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <zookeeper/zookeeper.h>

#include "leader.h"
#include "edge_betweenness.h"

using namespace std;

string HOSTNAME;                          // hostname of current program
string MY_IP = "";                        // ip address of current program 
string LEADER_IP = "";                    // ip address of cluster leader
string NODES_PATH = "/graph/test_nodes";  // path to file containing nodes
string EDGES_PATH = "/graph/test_edges";  // path to file containing edges
int PORT = 10000;                           // port to access leader server
// string CLUSTER = "";                      // name of cluster to which current program is assigned 

// size of eb cluster and nodes assigned to that cluster
int EB_CNT = 1;
string EB_NODES[] = {"node2"};

// size of mod cluster and nodes assigned to that cluster
int MOD_CNT = 1;
string MOD_NODES[] = {"node3"};

void setIPAdress() {
    /* funtion for setting clients IP address */

    struct hostent* host_entry = gethostbyname(&HOSTNAME[0]);

    if (host_entry == NULL) {
        cout << "ERROR: failed in getIPAdress()\n";
        exit(1);
    }

    char* IPbuffer = inet_ntoa(*(struct in_addr*)
                                    host_entry->h_addr_list[0]);

    MY_IP = IPbuffer;
}

void setHostName() {
    /* function for setting clients hostname */

    char buff[100];
    int hostname = gethostname(buff, sizeof(buff));

    HOSTNAME = buff;

    if (hostname == -1) {
        cout << "ERROR: failed in setHostName()\n";
        exit(1);
    }
}

bool checkCluster(zhandle_t* zh, string path) {
    /* checks if client is assigned to particular
        cluster identified with path */

    struct String_vector children;
    int r = zoo_get_children(zh, &path[0], 0, &children);
    if (r != ZOK) {
        cout << "ERROR: failed in checkCluster()\n";
        exit(1);
    }

    for (int i = 0; i < children.count; i++) {
        if (HOSTNAME == children.data[i]) return true;
    }
    return false;
}

void createCluster(zhandle_t* zh, string cluster_name) {
    /* function that creates znodes for a given cluster */

    int i, r;
    string path, buff, message = "wait";
    if (cluster_name == "eb") {

        r = zoo_create(zh, "/eb", &message[0], (int)message.size(), &ZOO_OPEN_ACL_UNSAFE, 
            ZOO_PERSISTENT, &buff[0], sizeof(buff));
        if (r != ZOK) cout << "ERROR (" << r << "): can't create /eb \n";

        for (i = 0; i < EB_CNT; i++) {
            path = "/eb/";
            path += EB_NODES[i];
            r = zoo_create(zh, &path[0], &MY_IP[0], (int)MY_IP.size(), &ZOO_OPEN_ACL_UNSAFE, 
                ZOO_PERSISTENT, &buff[0], sizeof(buff));
            if (r != ZOK) cout << "ERROR (" << r << "): can't create node in /eb \n";
        }
    }
    else if (cluster_name == "mod") {
        r = zoo_create(zh, "/mod", &message[0], (int)message.size(), &ZOO_OPEN_ACL_UNSAFE, 
            ZOO_PERSISTENT, &buff[0], sizeof(buff));
        if (r != ZOK) cout << "ERROR (" << r << "): can't create /mod \n";

        for (i = 0; i < MOD_CNT; i++) {
            path = "/mod/";
            path += MOD_NODES[i];
            r = zoo_create(zh, &path[0], &MY_IP[0], (int)MY_IP.size(), &ZOO_OPEN_ACL_UNSAFE, 
                ZOO_PERSISTENT, &buff[0], sizeof(buff));
            if (r != ZOK) cout << "ERROR (" << r << "): can't create node in /mod \n";
        }
    }
}

int main() 
{
    zoo_set_debug_level(ZOO_LOG_LEVEL_DEBUG);

    // initiating zookeeper connection
    zhandle_t *zkHandler = zookeeper_init("localhost:2181", NULL, 1000, 0, 0, 0);

    if (zkHandler == NULL) {
        cout << "Failed to connect to Zookeeper\n";
        exit(1);
    }
    cout << "Connected sucessfully!\n";

    setHostName();
    setIPAdress();

    // creating /max znode
    char buff[256];
    int r = zoo_create(zkHandler, "/max", &HOSTNAME[0], (int)HOSTNAME.size(),
        &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT, buff, sizeof(buff));

    if (r == ZOK) {
        // since this client managed to create /max znode, it gets to be the leader 

        cout << "CREATED NODE (" << buff << "): leader initialized\n";
        r = zoo_set(zkHandler, "/max", &MY_IP[0], (int)MY_IP.size(), -1);
        LEADER_IP = MY_IP;

        // creating /eb and client znodes (izdvojiti u posebnu funkciju)
        createCluster(zkHandler, "eb");
        cout << "Leader created /eb\n";

        // creating /mod and client znodes (izdvojiti u posebnu funkciju)
        createCluster(zkHandler, "mod");
        cout << "Leader created /mod\n";

        // initiate a server
        int server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0) {
            cout << "ERROR: can't open server socket\n";
            exit(1);
        }

        sockaddr_in server_address;
        int server_addrlen = sizeof(server_address);

        bzero((char *) &server_address, server_addrlen);
        server_address.sin_family = AF_INET;
        server_address.sin_addr.s_addr = INADDR_ANY;
        server_address.sin_port = htons(PORT);

        r = bind(server_socket, (sockaddr*)&server_address, server_addrlen);
        if (r < 0) {
            cout << "ERROR (" << errno <<"): can't bind socket to port \n";
            exit(1);
        }

        r = listen(server_socket, 2);
        if (r < 0) {
            cout << "ERROR: listen failed\n";
            exit(1);
        }

        // FIX: lider ovde zabode
        int connection_socket = accept(server_socket, 
            (sockaddr*)&server_address, (socklen_t*)&server_addrlen);
        if (connection_socket < 0) {
            cout << "ERROR: accept failed\n";
            exit(1); 
        }

        /* main leader algorithm */
        Leader leader(NODES_PATH, EDGES_PATH);
 //       sleep(20);
        // start clusters
        leader.start_eb_cluster(zkHandler);
        leader.start_mod_cluster(zkHandler);

        int edge_to_delete, iteration = 1;
        while(leader.graph.num_edges) {
            cout << "In loop...\n";
            edge_to_delete = leader.find_central_edge(connection_socket);
            if (edge_to_delete == -1) break;
            leader.edges_to_delete.push({edge_to_delete, iteration});
            leader.calculate_modularity();
            ++iteration;
        }

        cout << "Completed!\n";

        close(server_socket);
        close(connection_socket);

    }
    else {
        // this client failed to create /max znode, therfore it is a worker 

        // set leader ip
        int len = sizeof(LEADER_IP);
        char ip_buff[len] = "";
        r = zoo_get(zkHandler, "/max", 0, ip_buff, &len, NULL);
        if (r != ZOK) {
            cout << "ERROR (" << r << "): can't get leader ip \n";
            exit(1);
        }
        LEADER_IP = ip_buff;

        cout << "Leader is: " << LEADER_IP << "\n";
        
        // check to which cluster does this worker belong
        if (checkCluster(zkHandler, "/eb")) {
            cout << HOSTNAME << " is in /eb cluster\n";

            string path_to_node = "/eb/" + HOSTNAME;
            r = zoo_set(zkHandler, &path_to_node[0], &MY_IP[0], (int)MY_IP.size(), -1);

            /* ----- main edge betweenness algorithm  ----- */
            EdgeWorker ew(NODES_PATH, EDGES_PATH); 

            char node_msg[20];
            // wait unit leader gives mark to start
            do {
                len = sizeof(node_msg);
                r = zoo_get(zkHandler, "/eb", 0, node_msg, &len, NULL);
                if (r != ZOK) { 
                    cout << "ERROR (" << r << "): can't get value from /eb \n";
                    exit(1);
                }
                sleep(1);
            } while (strcmp(node_msg, "wait") == 0);

            // setup client for communication with leader 
            int client_socket = socket(AF_INET, SOCK_STREAM, 0);
            if (client_socket < 0) {
                cout << "ERROR: can't create client socket\n";
                exit(1);
            }

            sockaddr_in leader_address;
            leader_address.sin_family = AF_INET;
            leader_address.sin_port = htons(PORT);

            // convert IPv4 from text to binary form
            r = inet_pton(AF_INET, &LEADER_IP[0], &leader_address.sin_addr);
            if (r <= 0) {
                cout << "ERROR (" << r << "): invalid address\n";
            } 

            r = connect(client_socket, (sockaddr*) &leader_address, sizeof(leader_address));
            if (r < 0) {
                cout << "ERROR (" << errno << "): can't connect to leader server\n";
                exit(1);
            }

            cout << "Connection successfull!\n";

            // implement edge betweenness calc  
            int most_central_edge;
            while(ew.graph.num_edges) {
                most_central_edge = ew.calculate_edge_betweenness(1, ew.graph.num_nodes); 

                // send data to leader
                r = write(client_socket, &most_central_edge, sizeof(int));
                if (r < 0) {
                    cout << "ERROR: failed sending message to server\n";
                    exit(1);
                }

                cout << "Removing " << most_central_edge << "...\n";
                ew.remove_edge(most_central_edge);
            }
            cout << "DONE!\n";
            int tmp = -1;
            r = write(client_socket, &(tmp), sizeof(int));

        }
        else if (checkCluster(zkHandler, "/mod")) {
            cout << HOSTNAME << " is in /mod cluster\n";

            string path_to_node = "/mod/" + HOSTNAME;
            r = zoo_set(zkHandler, &path_to_node[0], &MY_IP[0], (int)MY_IP.size(), -1);
        }
        else {
            cout << "ERROR: no cluster found!\n";
        }
    }

    zookeeper_close(zkHandler);

    return 0;

}