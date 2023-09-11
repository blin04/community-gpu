#include <stdio.h>
#include <iostream>
#include <fstream>
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
#include "modularity.h"

using namespace std;

string HOSTNAME;                          // hostname of current program
string MY_IP = "";                        // ip address of current program 
string LEADER_IP = "";                    // ip address of leader
string EB_CLUSTER_IP = "";                // ip address of eb cluster leader 
string MOD_CLUSTER_IP = "";               // ip address of mod cluster leader
string NODES_PATH = "/graph/nodes";  // path to file containing nodes
string EDGES_PATH = "/graph/edges";  // path to file containing edges
int PORT = 10000;                           // port to access leader server

// enum used for tracking in which state algorithm is
enum algo_state {
    WAITING,
    CONNECTED,
    STARTED
} STATE;

// size of eb cluster and nodes assigned to that cluster
int EB_CNT = 1;
string EB_NODES[] = {"node2"};

// size of mod cluster and nodes assigned to that cluster
int MOD_CNT = 1;
string MOD_NODES[] = {"node3"};

void setIPAdress() {
    /* funtion for setting IP address */

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
    /* function for setting hostname */

    char buff[100];
    int hostname = gethostname(buff, sizeof(buff));

    HOSTNAME = buff;

    if (hostname == -1) {
        cout << "ERROR: failed in setHostName()\n";
        exit(1);
    }
}

void setClustersIP(zhandle_t *zh) {
    /* 
    * function for setting IP addresses of cluster leaders 
    */
    // wait unit leader gives mark to start
    string eb_state = "", mod_state = "";
    char node_msg[20];
    int len, r;
    do {
        len = sizeof(node_msg);
        r = zoo_get(zh, "/eb", 0, node_msg, &len, NULL);
        if (r != ZOK) { 
            cout << "ERROR (" << r << "): can't get value from /eb \n";
            exit(1);
        }
        eb_state = node_msg;

        node_msg[0] = '\0';
        r = zoo_get(zh, "/mod", 0, node_msg, &len, NULL);
        if (r != ZOK) { 
            cout << "ERROR (" << r << "): can't get value from /eb \n";
            exit(1);
        }
        mod_state = node_msg;

        //cout << "STRINGS: " << eb_state << "|" << mod_state << "\n";

        sleep(1);
    } while (strncmp(&eb_state[0], "wait", 4) != 0 
        || strncmp(&mod_state[0], "wait", 4) != 0);

    len = 20;
    char ip_buff[len] = "";
    string eb_path = "/eb/" + EB_NODES[0];
    r = zoo_get(zh, &eb_path[0], 0, ip_buff, &len, NULL);
    if (r < 0) {
        cout << "ERROR: can't get EB cluster IP address\n";
        exit(1);
    }
    EB_CLUSTER_IP = ip_buff;

    string mod_path = "/mod/" + MOD_NODES[0];
    r = zoo_get(zh, &mod_path[0], 0, ip_buff, &len, NULL);
    if (r < 0) {
        cout << "ERROR: can't get MOD cluster IP address\n";
        exit(1);
    }
    MOD_CLUSTER_IP = ip_buff;

    cout << "--- CLUSTERS ---\n";
    cout << EB_CLUSTER_IP << "\n";
    cout << MOD_CLUSTER_IP << "\n";

    MOD_CLUSTER_IP = ip_buff;
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
    string path, buff;
    if (cluster_name == "eb") {

        r = zoo_create(zh, "/eb", "", 0, &ZOO_OPEN_ACL_UNSAFE, 
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
        r = zoo_create(zh, "/mod", "", 1, &ZOO_OPEN_ACL_UNSAFE, 
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
        // this client managed to create /max znode, so it becomes the leader 

        cout << "CREATED NODE (" << buff << "): leader initialized\n";
        r = zoo_set(zkHandler, "/max", &MY_IP[0], (int)MY_IP.size(), -1);
        LEADER_IP = MY_IP;

        // creating /eb and client znodes 
        createCluster(zkHandler, "eb");
        cout << "Leader created /eb\n";


        // algorithm is in waiting state
        STATE = WAITING;

        // creating /mod and client znodes
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

        // variables used for monitoring two socket descriptors
        fd_set readfds;
        int eb_socket = 0, mod_socket = 0, new_socket, activity;

        /* main leader algorithm */
        Leader leader(NODES_PATH, EDGES_PATH);

        setClustersIP(zkHandler);

        leader.start_eb_cluster(zkHandler);
        leader.start_mod_cluster(zkHandler);

        int most_central_edge, iteration = 1;
        double mod;
        cout << "About to begin\n";
        ofstream log("results.log");
        int max_sd = server_socket;
        while(!leader.check_if_finished(zkHandler)) {

            // establish connection with /eb and /mod clusters
            FD_ZERO(&readfds);
            FD_SET(server_socket, &readfds);
            if (eb_socket != 0) FD_SET(eb_socket, &readfds);
            if (mod_socket != 0) FD_SET(mod_socket, &readfds);

            max_sd = max(max_sd, eb_socket);
            max_sd = max(max_sd, mod_socket);

            activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
            
            if (STATE == WAITING && FD_ISSET(server_socket, &readfds)) {
                // this means that cluster leaders 
                // haven't connected to leader

                cout << "Connecting...\n";
                new_socket = accept(server_socket, 
                    (sockaddr*)&server_address, (socklen_t*)&server_addrlen);
                if (new_socket < 0) {
                    cout << "ERROR (" << errno << "): failed accepting connection\n";
                    exit(1);
                }
                cout << "Connection successful!\n";

                // check ip address of client that has just connected
                // to see which cluster leader he is

                char *cli_addr = inet_ntoa(server_address.sin_addr);
                cout << "Client connected " << cli_addr << "\n";
                if (strcmp(cli_addr, &EB_CLUSTER_IP[0]) == 0) {
                    eb_socket = new_socket;
                }
                else if (strcmp(cli_addr, &MOD_CLUSTER_IP[0]) == 0) {
                    mod_socket = new_socket;
                }

                if (eb_socket != 0 && mod_socket != 0) STATE = CONNECTED;
                continue;
            }

            // actual leader algorithm
            most_central_edge = leader.find_central_edge(eb_socket);
            if (most_central_edge == -1) break;

            leader.removal_order.push_back({iteration, most_central_edge});
            leader.edges_to_delete.push(most_central_edge);

            mod = leader.calculate_modularity(mod_socket);
            if (mod == -1) break;

            log << "--- ITERATION " << iteration << " ---\n";
            log << "Edge deleted: " << most_central_edge << "\n";
            log << "Modularity: " << mod << "\n";
            
            leader.modularity_values.push_back(mod);
            ++iteration;
        }

        cout << "Done!\n";

        // find highest modularity value 
        int best_it = -1;
        double best_q = -1.0;
        for (int i = 0; i < (int)leader.modularity_values.size(); i++) {
            if(leader.modularity_values[i] > best_q) {
                best_q = leader.modularity_values[i];
                best_it = i + 1;
            }
        }

        for (int i = 0; i < best_it; i++) {
            leader.graph.remove_edge(leader.removal_order[i].second);
        }

        vector<int> communities(leader.graph.num_nodes + 1);
        leader.graph.get_communities(communities);

        cout << "----- CALCULATED COMMUNITIES -----\n";
        for (auto c : communities) {
            cout << c << " ";
        }
        cout << "\n";

        close(server_socket);
        close(eb_socket);
        close(mod_socket);
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
        if (r != ZOK) {
            cout << "ERROR (" << errno << "): can't connect to leader server\n";
            exit(1);
        }           
        cout << "Connection with server successful!\n";

        // check to which cluster does this worker belong
        if (checkCluster(zkHandler, "/eb")) {
            cout << HOSTNAME << " is in /eb cluster\n";

            string path_to_node = "/eb/" + HOSTNAME;
            r = zoo_set(zkHandler, &path_to_node[0], &MY_IP[0], (int)MY_IP.size(), -1);
            r = zoo_set(zkHandler, "/eb", "wait", 4, -1);

            /* ----- main edge betweenness algorithm  ----- */
            EdgeWorker ew(NODES_PATH, EDGES_PATH); 

            // wait unit leader gives mark to start
            char node_msg[20];
            do {
                len = sizeof(node_msg);
                r = zoo_get(zkHandler, "/eb", 0, node_msg, &len, NULL);
                if (r != ZOK) { 
                    cout << "ERROR (" << r << "): can't get value from /eb \n";
                    exit(1);
                }
            } while (strcmp(node_msg, "wait") == 0);

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
                ew.graph.remove_edge(most_central_edge);
            }
            cout << "Done!\n";
            int tmp = -1;
            write(client_socket, &tmp, sizeof(int));

            // mark to leader that algorithm has finished
            string msg = "finished";
            r = zoo_set(zkHandler, "/eb", &msg[0], (int)msg.size(), -1);
        }
        else if (checkCluster(zkHandler, "/mod")) {
            cout << HOSTNAME << " is in /mod cluster\n";

            string path_to_node = "/mod/" + HOSTNAME;
            r = zoo_set(zkHandler, &path_to_node[0], &MY_IP[0], (int)MY_IP.size(), -1);
            r = zoo_set(zkHandler, "/mod", "wait", 4, -1);

            /* ----- main modularity algorithm ----- */
            ModulWorker mw(NODES_PATH, EDGES_PATH);
    
            // wait unit leader gives mark to start
            char node_msg[20];
            do {
                len = sizeof(node_msg);
                r = zoo_get(zkHandler, "/mod", 0, node_msg, &len, NULL);
                if (r != ZOK) { 
                    cout << "ERROR (" << r << "): can't get value from /mod \n";
                    exit(1);
                }
            } while (strcmp(node_msg, "wait") == 0);

            // calculate modularity
            double q;
            int edge_to_delete;
            Graph graph(NODES_PATH, EDGES_PATH);
            vector<int> comm(graph.num_nodes + 1);
            while(graph.num_edges) {
                r = read(client_socket, &edge_to_delete, sizeof(int));
                if (r < 0) {
                    cout << "ERROR (" << errno <<  "): failed reading from leader\n";
                    exit(1);
                }
                graph.remove_edge(edge_to_delete);
                graph.get_communities(comm);
                q = mw.calculate_modularity(1, mw.graph.num_nodes, comm);
                r = write(client_socket, &q, sizeof(double));
                if (r < 0) {
                    cout << "ERROR (" << errno <<  "): failed writing to leader\n";
                    exit(1);
                }
            }
            cout << "Done!\n";
            int tmp = -1;
            write(client_socket, &tmp, sizeof(int));

            // mark to leader that algorithm has finished
            string msg = "finished";
            r = zoo_set(zkHandler, "/mod", &msg[0], (int)msg.size(), -1);
        }
        else {
            cout << "ERROR: no cluster found!\n";
        }
    }

    zookeeper_close(zkHandler);

    return 0;

}