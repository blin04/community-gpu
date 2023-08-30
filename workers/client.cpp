#include <stdio.h>
#include <iostream>
#include <string>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
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
    string path, buff;
    if (cluster_name == "eb") {

        r = zoo_create(zh, "/eb", &MY_IP[0], (int)MY_IP.size(), &ZOO_OPEN_ACL_UNSAFE, 
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
        r = zoo_create(zh, "/mod", &MY_IP[0], (int)MY_IP.size(), &ZOO_OPEN_ACL_UNSAFE, 
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
        return errno;
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

        /* ----- main leader algorithm ----- */
        Leader leader("/graph/nodes", "/graph/edges");

        /*while(leader.graph.num_edges) {
            leader.find_central_edge();
            leader.calculate_modularity();
        }*/
    }
    else {
        // this client failed to create /max znode, therfore it is a worker 

        // set leader ip
        int len = sizeof(LEADER_IP);
        char buff[len] = "";
        r = zoo_get(zkHandler, "/max", 0, buff, &len, NULL);
        if (r != ZOK) cout << "ERROR (" << r << "): can't get leader ip \n";
        LEADER_IP = buff;

        cout << "Leader is: " << LEADER_IP << "\n";
        
        // check to which cluster does this worker belong
        if (checkCluster(zkHandler, "/eb")) {
            cout << HOSTNAME << " is in /eb cluster\n";

            string path_to_node = "/eb/" + HOSTNAME;
            r = zoo_set(zkHandler, &path_to_node[0], &MY_IP[0], (int)MY_IP.size(), -1);

            /* ----- main edge betweenness algorithm  ----- */
            EdgeWorker ew("/graph/nodes", "/graph/edges"); 

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