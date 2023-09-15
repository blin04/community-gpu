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

#include "../includes/leader.h"
#include "../includes/edge_betweenness.h"
#include "../includes/modularity.h"
#include "../includes/globals.h"

using namespace std;

string HOSTNAME;                          // hostname of current program
string MY_IP = "";                        // ip address of current program 
string LEADER_IP = "";                    // ip address of leader
string EB_CLUSTER_IP = "";                // ip address of eb cluster leader 
string MOD_CLUSTER_IP = "";               // ip address of mod cluster leader

int MAIN_PORT = 10000;                    // port to access main leader server
int CLUSTER_PORT = 11000;                 // port to access cluster leaders

// enum used for tracking in which state algorithm is
enum algo_state {
    WAITING,
    CONNECTED,
    STARTED
} STATE;

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

void setClustersIP(zhandle_t *zh) {
    /* 
    * function for setting IP addresses of cluster leaders 
    * (used by main leader only)
    */

    string eb_state = "", mod_state = "";
    char node_msg[20];
    int len, r;

    // waiting until cluster leaders become ready
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

    } while (strncmp(&eb_state[0], "wait", 4) != 0 
        || strncmp(&mod_state[0], "wait", 4) != 0);

    // after cluster leaders became ready, read
    // their IP addresses from corresponding znodes
    len = 20;
    char ip_buff[len] = "";
    string eb_path = "/eb/" + EB_NODES[0];
    r = zoo_get(zh, &eb_path[0], 0, ip_buff, &len, NULL);
    if (r != ZOK) {
        cout << "ERROR: can't get EB cluster IP address\n";
        exit(EXIT_FAILURE);
    }
    EB_CLUSTER_IP = ip_buff;

    string mod_path = "/mod/" + MOD_NODES[0];
    r = zoo_get(zh, &mod_path[0], 0, ip_buff, &len, NULL);
    if (r != ZOK) {
        cout << "ERROR: can't get MOD cluster IP address\n";
        exit(EXIT_FAILURE);
    }
    MOD_CLUSTER_IP = ip_buff;

    //cout << "--- CLUSTERS ---\n";
    //cout << EB_CLUSTER_IP << "\n";
    //cout << MOD_CLUSTER_IP << "\n";
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

void createCluster(zhandle_t* zh, string cluster_name) {
    /* 
    * function that creates znodes for a given cluster 
    * (used only by main leader)
    */

    int i, r;
    string path, buff;
    if (cluster_name == "eb") {

        r = zoo_create(zh, "/eb", "", 0, &ZOO_OPEN_ACL_UNSAFE, 
            ZOO_PERSISTENT, &buff[0], sizeof(buff));
        if (r != ZOK) 
            cout << "ERROR (" << r << "): can't create /eb \n";

        for (i = 0; i < EB_CNT; i++) {
            path = "/eb/";
            path += EB_NODES[i];
            r = zoo_create(zh, &path[0], &MY_IP[0], (int)MY_IP.size(), &ZOO_OPEN_ACL_UNSAFE, 
                ZOO_PERSISTENT, &buff[0], sizeof(buff));
            if (r != ZOK) 
                cout << "ERROR (" << r << "): can't create node in /eb \n";
        }
    }
    else if (cluster_name == "mod") {
        r = zoo_create(zh, "/mod", "", 1, &ZOO_OPEN_ACL_UNSAFE, 
            ZOO_PERSISTENT, &buff[0], sizeof(buff));
        if (r != ZOK) 
            cout << "ERROR (" << r << "): can't create /mod \n";

        for (i = 0; i < MOD_CNT; i++) {
            path = "/mod/";
            path += MOD_NODES[i];
            r = zoo_create(zh, &path[0], &MY_IP[0], (int)MY_IP.size(), &ZOO_OPEN_ACL_UNSAFE, 
                ZOO_PERSISTENT, &buff[0], sizeof(buff));
            if (r != ZOK) 
                cout << "ERROR (" << r << "): can't create node in /mod \n";
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
        exit(EXIT_FAILURE);
    }
    cout << "Connected sucessfully!\n";

    // set necessary global variables
    setHostName();
    setIPAdress();

    // try to create /max znode
    char buff[256];
    int r = zoo_create(zkHandler, "/max", &HOSTNAME[0], (int)HOSTNAME.size(),
        &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT, buff, sizeof(buff));

    if (r == ZOK) {
        /*
        * this machine managed to create /max znode, 
        * therfore it becomes main leader
        */

        cout << "CREATED NODE (" << buff << "): leader initialized\n";
        r = zoo_set(zkHandler, "/max", &MY_IP[0], (int)MY_IP.size(), -1);
        if (r != ZOK) {
            cout<< "ERROR: failed to set address into /max znode\n";
        }
        LEADER_IP = MY_IP;

        // create znodes for /eb cluster
        createCluster(zkHandler, "eb");
        cout << "Leader created /eb\n";

        // create znodes for /mod cluster
        createCluster(zkHandler, "mod");
        cout << "Leader created /mod\n";

        // algorithm is in waiting state
        STATE = WAITING;

        // create socket for communication with clusters
        int server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0) {
            cout << "ERROR: can't open server socket\n";
            exit(EXIT_FAILURE);
        }

        sockaddr_in server_address;
        int server_addrlen = sizeof(server_address);

        bzero((char *) &server_address, server_addrlen);
        server_address.sin_family = AF_INET;
        server_address.sin_addr.s_addr = INADDR_ANY;
        server_address.sin_port = htons(MAIN_PORT);

        // bind socket with IP address of server
        r = bind(server_socket, (sockaddr*)&server_address, server_addrlen);
        if (r < 0) {
            cout << "ERROR (" << errno <<"): can't bind socket to port \n";
            exit(EXIT_FAILURE);
        }

        // listen for new connections
        r = listen(server_socket, 2);
        if (r < 0) {
            cout << "ERROR (" << errno << "): listen failed\n";
            exit(EXIT_FAILURE);
        }

        // variables used for monitoring cluster socket descriptors
        fd_set readfds;
        int eb_socket = 0, mod_socket = 0, new_socket, activity;

        Leader leader(NODES_PATH, EDGES_PATH);

        // set IP addresses of cluster leaders
        setClustersIP(zkHandler);

        // start clusters
        leader.start_eb_cluster(zkHandler);
        leader.start_mod_cluster(zkHandler);

        // some needed variables
        int most_central_edge, iteration = 1;
        double mod;
        int max_sd = server_socket;

        // file used for logging intermediate results of algorithm
        ofstream log("/project/results.log");         

        while(!leader.check_if_finished(zkHandler)) {

            // establish connection with /eb and /mod clusters
            FD_ZERO(&readfds);
            FD_SET(server_socket, &readfds);
            if (eb_socket != 0) FD_SET(eb_socket, &readfds);
            if (mod_socket != 0) FD_SET(mod_socket, &readfds);

            max_sd = max(max_sd, eb_socket);
            max_sd = max(max_sd, mod_socket);

            // for now timeval is NULL, which means this will wait forever 
            // maybe fix this later
            activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
            
            // if the system is in waiting state, it means that 
            // some machines still haven't connected to leader
            if (STATE == WAITING && FD_ISSET(server_socket, &readfds)) {

                new_socket = accept(server_socket, 
                    (sockaddr*)&server_address, (socklen_t*)&server_addrlen);
                if (new_socket < 0) {
                    cout << "ERROR (" << errno << "): failed accepting connection\n";
                    exit(EXIT_FAILURE);
                }

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

                // if all machines have connected, switch to new state
                if (eb_socket != 0 && mod_socket != 0) STATE = CONNECTED;
                continue;
            }

            /*
            * here begins main leader loop
            */ 

            // get id of an edge that has highest value of centrality
            most_central_edge = leader.find_central_edge(eb_socket);
            // if (most_central_edge == -1) break;

            // update removal order 
            leader.removal_order.push_back({iteration, most_central_edge});
            leader.edges_to_delete.push(most_central_edge);

            // calculate value of modularity for current partition
            mod = leader.calculate_modularity(mod_socket);
            // if (mod == -1) break;
            leader.modularity_values.push_back(mod);

            // log intermediate results
            log << "--- ITERATION " << iteration << " ---\n";
            log << "Edge deleted: " << most_central_edge << "\n";
            log << "Modularity: " << mod << "\n";
            
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

        // remove all edges until partition with
        // highest modularity is reached
        for (int i = 0; i < best_it; i++) {
            leader.graph.remove_edge(leader.removal_order[i].second);
        }

        vector<int> communities(leader.graph.num_nodes + 1);
        leader.graph.get_communities(communities);

        // print out result of the algorithm 
        cout << "----- ASSIGNED COMMUNITIES -----\n";
        for (auto c : communities) {
            cout << c << " ";
        }
        cout << "\n";

        close(server_socket);
        close(eb_socket);
        close(mod_socket);
    }
    else {
        /*
        * this machine failed to create /max znode, therefore
        * it becomes a leader for a particular cluster
        */
        cout << "My ip: " << MY_IP << "\n";

        // set ip address of main leader
        int len = sizeof(LEADER_IP);
        char ip_buff[len] = "";
        r = zoo_get(zkHandler, "/max", 0, ip_buff, &len, NULL);
        if (r != ZOK) {
            cout << "ERROR (" << r << "): can't get leader ip \n";
            exit(EXIT_FAILURE);
        }
        LEADER_IP = ip_buff;
        cout << "Main Leader IP is: " << LEADER_IP << "\n";

        // create socket for communication with main leader 
        int client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (client_socket < 0) {
            cout << "ERROR: can't create client socket\n";
            exit(EXIT_FAILURE);
        }

        // struct containing address of main leader
        sockaddr_in main_leader_addr;
        main_leader_addr.sin_family = AF_INET;
        main_leader_addr.sin_port = htons(MAIN_PORT);

        // convert IPv4 address of main leader
        // from text to binary form
        r = inet_pton(AF_INET, &LEADER_IP[0], &main_leader_addr.sin_addr);
        if (r <= 0) {
            cout << "ERROR (" << r << "): invalid address\n";
            exit(EXIT_FAILURE);
        } 

        // connect to main leader
        r = connect(client_socket, (sockaddr*) &main_leader_addr, sizeof(main_leader_addr));
        if (r < 0) {
            cout << "ERROR (" << errno << "): can't connect to leader server\n";
            exit(EXIT_FAILURE);
        }           
        cout << "Connection with server successful!\n";


        // create socket for communication with workers 
        int server_socket = socket(AF_INET, SOCK_STREAM, 0);

        // struct containing address of cluster leader
        sockaddr_in cluster_leader_addr;
        cluster_leader_addr.sin_family = AF_INET;
        cluster_leader_addr.sin_addr.s_addr = INADDR_ANY;
        cluster_leader_addr.sin_port = htons(CLUSTER_PORT);
        int cluster_leader_addrlen = sizeof(cluster_leader_addr);

        r = bind(server_socket, (sockaddr*)&cluster_leader_addr, sizeof(cluster_leader_addr));
        if (r < 0) {
            cout << "ERROR (" << errno << "): can't bind address to cluster leader\n";
            exit(EXIT_FAILURE);
        }

        // check which cluster is this machine assigned to
        if (checkCluster(zkHandler, "/eb")) {
            cout << HOSTNAME << " is in /eb cluster\n";

            // set necessary znodes
            string path_to_node = "/eb/" + HOSTNAME;
            r = zoo_set(zkHandler, &path_to_node[0], &MY_IP[0], (int)MY_IP.size(), -1);
            if (r != ZOK) {
                cout << "ERROR (" << errno << "): can't set /eb leaders IP address to corresponding znode \n";
                exit(EXIT_FAILURE);
            }
            r = zoo_set(zkHandler, "/eb", "wait", 4, -1);
            if (r != ZOK) {
                cout << "ERROR (" << errno << "): can't set state of /eb cluster \n";
                exit(EXIT_FAILURE);
            }

            // set limit for amount of connections
            r = listen(server_socket, EB_CNT);
            if (r < 0) {
                cout << "ERROR (" << errno <<"): listen failed for /eb\n";
                exit(EXIT_FAILURE);
            }

            //struct for monitoring worker sockets 
            fd_set readfds;
            int max_sd, activity, new_socket;

            // struct used for timeout on select() sys call
            timeval tv;
            tv.tv_sec = 3;
            tv.tv_usec = 0;

            // array of worker sockets
            int workers[EB_CNT - 1];
            fill(workers, workers + EB_CNT - 1, -1);

            int num_of_connected = 0;
            while(num_of_connected != EB_CNT - 1) {
                /*
                * in this loop cluster leader waits until all workers have connected to him
                */
                FD_ZERO(&readfds);
                
                FD_SET(server_socket, &readfds);
                max_sd = server_socket;

                for (int i = 0; i < EB_CNT - 1; i++) {
                    if (workers[i] != -1) {
                        max_sd = max(max_sd, workers[i]);
                        FD_SET(workers[i], &readfds);
                    }
                }

                activity = select(max_sd + 1, &readfds, NULL, NULL, &tv);

                if (FD_ISSET(server_socket, &readfds)) {
                    // some worker is trying to connect
                    new_socket = accept(server_socket, 
                        (sockaddr*)&cluster_leader_addr, (socklen_t*)&cluster_leader_addrlen);
                    if (new_socket < 0) {
                        cout << "ERROR (" << strerror(errno) << "): failed accepting connection from worker\n";
                        exit(EXIT_FAILURE);
                    }
                    cout << "Connected with worker\n";

                    // store new_socket in array
                    workers[num_of_connected++] = new_socket;
                }
            }
            cout << "Connected all workers!\n";

            // wait until leader gives mark to start calculating
            char node_msg[20];
            do {
                len = sizeof(node_msg);
                r = zoo_get(zkHandler, "/eb", 0, node_msg, &len, NULL);
                if (r != ZOK) { 
                    cout << "ERROR (" << r << "): can't get value from /eb \n";
                    exit(EXIT_FAILURE);
                }
            } while (strcmp(node_msg, "wait") == 0);

            // calculate edge betweenness
            Graph graph(NODES_PATH, EDGES_PATH);
            bool workers_finished = true;
            vector<double> betweenness(graph.num_edges + 1), input_betweenness(graph.num_edges + 1);

            // split node range into intervals, 
            // each worker is assigned one of them 
            vector<pair<int, int>> intervals;
            int k = graph.num_nodes / (EB_CNT - 1);
            int r = graph.num_nodes % (EB_CNT - 1);

            int start = 1, end;
            while(start < graph.num_nodes) {
                end = start + (k - 1); 
                if (r) {
                    ++end;
                    --r;
                }
                intervals.push_back({start, end});
                start = end + 1;
            }

            if ((int)intervals.size() != EB_CNT - 1) {
                cout << "ERROR: failed splitting intervals\n";
                exit(EXIT_FAILURE);
            }

            cout << "About to begin!\n";
            while(graph.num_edges) {
                if (workers_finished) {
                    // start workers
                    for (int i = 0; i < EB_CNT - 1; i++) {
                        r = write(workers[i], &intervals[i].first, sizeof(intervals[i].first));
                        if (r < 0) {
                            cout << "ERROR (" << errno << "): failed writing to worker\n";
                            exit(EXIT_FAILURE);
                        }
                        r = write(workers[i], &intervals[i].second, sizeof(intervals[i].second));
                        if (r < 0) {
                            cout << "ERROR (" << errno << "): failed writing to worker\n";
                            exit(EXIT_FAILURE);
                        }
                    }
                    workers_finished = false;
                }

                // add worker sockets to fd_set
                FD_ZERO(&readfds);

                FD_SET(server_socket, &readfds);
                max_sd = server_socket;
                for (int i = 0; i < EB_CNT - 1; i++) {
                    FD_SET(workers[i], &readfds);
                    max_sd = max(max_sd, workers[i]);
                }

                // check if all workers finished
                activity = select(max_sd + 1, &readfds, NULL, NULL, &tv);

                workers_finished = true; 
                for (int i = 0; i < EB_CNT - 1; i++) {
                    if (!FD_ISSET(workers[i], &readfds)) {
                        workers_finished = false;
                    }
                }

                if (workers_finished) {
                    // read results from workers
                    fill(betweenness.begin(), betweenness.end(), 0);
                    for (int i = 0; i < EB_CNT - 1; i++) {
                        r = read(workers[i], input_betweenness.data(), 
                            input_betweenness.size() * sizeof(input_betweenness[0]));
                        if (r < 0) {
                            cout << "ERROR (" << errno << "): failed reading from worker!\n";
                            exit(EXIT_FAILURE);
                        }

                        for (int i = 1; i <= graph.orig_num_edges; i++) {
                            betweenness[i] += input_betweenness[i];    
                        }
                    }

                    // find edge with highest centrality value
                    // and remove it
                    int edge_to_delete;
                    double highest_betweenness = -1;
                    for (int i = 1; i <= graph.orig_num_edges; i++) {
                        if (betweenness[i] > highest_betweenness) {
                            highest_betweenness = betweenness[i];
                            edge_to_delete = i;
                        }
                    }
                    graph.remove_edge(edge_to_delete);

                    // send most central edge to main leader and to workers
                    r = write(client_socket, &edge_to_delete, sizeof(edge_to_delete));
                    if (r < 0) {
                        cout << "ERROR (" << errno << "): couldn't send result to main leader\n";
                    }

                    for (int i = 0; i < EB_CNT - 1; i++) {
                        r = write(workers[i], &edge_to_delete, sizeof(edge_to_delete));
                        if (r < 0) {
                            cout << "ERROR (" << errno << "): couldn't send result to workers\n";
                        }
                    }
                }
            }
            cout << "Done!\n";

            // mark to leader that cluster has finished
            string msg = "finished";
            r = zoo_set(zkHandler, "/eb", &msg[0], (int)msg.size(), -1);
            if (r != ZOK) {
                cout << "ERROR (" << errno << "): can't set state of /eb cluster \n";
                exit(EXIT_FAILURE);
            }

            // close socket connections
            for (int i = 0; i < EB_CNT - 1; i++) 
                close(workers[i]); 
        }
        else if (checkCluster(zkHandler, "/mod")) {
            cout << HOSTNAME << " is in /mod cluster\n";

            // set necessary znodes
            string path_to_node = "/mod/" + HOSTNAME;
            r = zoo_set(zkHandler, &path_to_node[0], &MY_IP[0], (int)MY_IP.size(), -1);
            if (r != ZOK) {
                cout << "ERROR (" << errno << "): can't set /mod leaders IP address to corresponding znode \n";
                exit(EXIT_FAILURE);
            }
            r = zoo_set(zkHandler, "/mod", "wait", 4, -1);
            if (r != ZOK) {
                cout << "ERROR (" << errno << "): can't set state of /mod cluster \n";
                exit(EXIT_FAILURE);
            }

            // set limit for amount of connections
            r = listen(server_socket, MOD_CNT);
            if (r < 0) {
                cout << "ERROR (" << errno <<"): listen failed for /mod\n";
                exit(EXIT_FAILURE);
            }

            //struct for monitoring worker sockets
            fd_set readfds;
            int max_sd, activity, new_socket;

            // struct used for timeout on select()
            timeval tv;
            tv.tv_sec = 3;
            tv.tv_usec = 0;

            // array of woker sockets
            int workers[MOD_CNT - 1];
            fill(workers, workers + MOD_CNT - 1, -1);

            int num_of_connected = 0;
            while(num_of_connected != MOD_CNT - 1) {
                /*
                * in this loop cluster leader waits until all workers have connected to him
                */
                FD_ZERO(&readfds);
                
                FD_SET(server_socket, &readfds);
                max_sd = server_socket;

                for (int i = 0; i < MOD_CNT - 1; i++) {
                    if (workers[i] != -1) {
                        max_sd = max(max_sd, workers[i]);
                        FD_SET(workers[i], &readfds);
                    }
                }

                activity = select(max_sd + 1, &readfds, NULL, NULL, &tv);

                if (FD_ISSET(server_socket, &readfds)) {
                    // some worker is trying to connect
                    new_socket = accept(server_socket, 
                        (sockaddr*)&cluster_leader_addr, (socklen_t*)&cluster_leader_addrlen);
                    if (new_socket < 0) {
                        cout << "ERROR (" << strerror(errno) << "): failed accepting connection from worker\n";
                        exit(EXIT_FAILURE);
                    }
                    cout << "Connected with worker\n";

                    // store new_socket in array
                    workers[num_of_connected++] = new_socket;
                }
            }
            cout << "Connected all workers!\n";

            // wait until leader gives mark to start calculating
            char node_msg[20];
            do {
                len = sizeof(node_msg);
                r = zoo_get(zkHandler, "/mod", 0, node_msg, &len, NULL);
                if (r != ZOK) { 
                    cout << "ERROR (" << r << "): can't get value from /mod \n";
                    exit(EXIT_FAILURE);
                }
            } while (strcmp(node_msg, "wait") == 0);

            // some variables used to calculate modularity 
            double total_q, q;
            int edge_to_delete;
            bool workers_finished = true;
            Graph graph(NODES_PATH, EDGES_PATH);
            vector<int> comm(graph.num_nodes + 1);

            // split node range into intervals, 
            // each worker is assigned one of them 
            vector<pair<int, int>> intervals;
            int k = graph.num_nodes / (MOD_CNT - 1);
            int r = graph.num_nodes % (MOD_CNT - 1);

            int start = 1, end;
            while(start < graph.num_nodes) {
                end = start + (k - 1); 
                if (r) {
                    ++end;
                    --r;
                }
                intervals.push_back({start, end});
                start = end + 1;
            }

            if ((int)intervals.size() != MOD_CNT - 1) {
                cout << "ERROR: failed splitting intervals\n";
                exit(EXIT_FAILURE);
            }

            // calculate modularity
            while(graph.num_edges) {
                if (workers_finished) {
                    // read edge that should be deleted and delete it
                    r = read(client_socket, &edge_to_delete, sizeof(edge_to_delete));
                    if (r < 0 ) {
                        cout << "ERROR (" << errno << "): failed reading edge to delete\n";
                        exit(EXIT_FAILURE);
                    }
                    graph.remove_edge(edge_to_delete);

                    // get communities of current partition
                    graph.get_communities(comm);

                    // start workers
                    for (int i = 0; i < MOD_CNT - 1; i++) {
                        // send starting node to worker
                        r = write(workers[i], &intervals[i].first, sizeof(intervals[i].first));
                        if (r < 0) {
                            cout << "ERROR: (" << errno << "): can't write to worker\n";
                            exit(EXIT_FAILURE);
                        }

                        // send ending node to worker
                        r = write(workers[i], &intervals[i].second, sizeof(intervals[i].second));
                        if (r < 0) {
                            cout << "ERROR: (" << errno << "): can't write to worker\n";
                            exit(EXIT_FAILURE);
                        }

                        // send vector of communities to worker
                        r = write(workers[i], comm.data(), comm.size() * sizeof(comm[0]));
                        if (r < 0) {
                            cout << "ERROR: (" << errno << "): can't send communities to worker\n";
                            exit(EXIT_FAILURE);
                        }
                    }
                    workers_finished = false;
                }

                // add sockes to fd_set
                FD_ZERO(&readfds);

                FD_SET(server_socket, &readfds);
                max_sd = server_socket;
                for (int i = 0; i < MOD_CNT - 1; i++) {
                    FD_SET(workers[i], &readfds);
                    max_sd = max(max_sd, workers[i]);
                }

                // check if workers finished
                activity = select(max_sd + 1, &readfds, NULL, NULL, &tv);

                workers_finished = true;
                for (int i = 0; i < MOD_CNT - 1; i++) {
                    if (!FD_ISSET(workers[i], &readfds)) {
                        workers_finished = false;
                    }
                }

                // read results from workers
                if (workers_finished) {
                    total_q = 0;
                    for (int i = 0; i < MOD_CNT - 1; i++) {
                        r = read(workers[i], &q, sizeof(q));
                        if (r < 0) {
                            cout << "ERROR (" << errno << "): failed reading from worker!\n";
                            exit(EXIT_FAILURE);
                        }
                        total_q += q;
                    }

                    // send result to main leader
                    r = write(client_socket, &total_q, sizeof(total_q));
                    if (r < 0) {
                        cout << "ERROR (" << errno << "): failed writing to main leader!\n";
                        exit(EXIT_FAILURE);
                    }

                }
            }
            cout << "Done!\n";

            // mark to leader that cluster has finished
            string msg = "finished";
            r = zoo_set(zkHandler, "/mod", &msg[0], (int)msg.size(), -1);

            // this is temporary solution, should fix later
            int tmp = -1;
            for (int i = 0; i < MOD_CNT - 1; i++) {
                write(workers[i], &tmp, sizeof(tmp));
            }

            // close socket connections
            for (int i = 0; i < MOD_CNT - 1; i++)
                close(workers[i]);
        }
        else {
            cout << "ERROR: no cluster found!\n";
        }
    }

    zookeeper_close(zkHandler);

    return 0;

}
