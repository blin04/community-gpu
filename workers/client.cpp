#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <zookeeper/zookeeper.h>

#define HOSTNAME_LEN 100

char HOSTNAME[HOSTNAME_LEN];                // hostname of current program
char MY_IP[32] = "";                        // ip address of current program 
char LEADER_IP[32] = "";                    // ip address of cluster leader
char CLUSTER[32] = "";                      // name of cluster to which current program is assigned 

// size of eb cluster and nodes assigned to that cluster
int EB_CNT = 3;
char *EB_NODES[] = {"node2", "node4", "node5"};

// size of mod cluster and nodes assigned to that cluster
int MOD_CNT = 2;
char *MOD_NODES[] = {"node3", "node6"};

void setIPAdress() {
    /* funtion for setting clients IP address */

    struct hostent* host_entry = gethostbyname(HOSTNAME);

    if (host_entry == NULL) {
        perror("ERROR: failed in getIPAdress()\n");
        exit(1);
    }

    char* IPbuffer = inet_ntoa(*(struct in_addr*)
                                    host_entry->h_addr_list[0]);

    strcpy(MY_IP, IPbuffer);
}

void setHostName() {
    /* function for setting clients hostname */

    int hostname = gethostname(HOSTNAME, HOSTNAME_LEN);

    if (hostname == -1) {
        perror("ERROR: failed in setHostName()\n");
        exit(1);
    }
}

bool checkCluster(zhandle_t* zh, char* path) {
    /* checks if client is assigned to particular
        cluster identified with path */

    struct String_vector children;
    int r = zoo_get_children(zh, path, 0, &children);
    if (r != ZOK) {
        printf("ERROR: failed in checkCluster()");
        exit(1);
    }

    for (int i = 0; i < children.count; i++) {
        if (strcmp(HOSTNAME, children.data[i]) == 0) return true;
    }
    return false;
}

void createCluster(zhandle_t* zh, char* cluster_name) {
    /* function that creates znodes for a given cluster */

    int i, r;
    char path[15];
    char buff[50];
    if (cluster_name == "eb") {

        r = zoo_create(zh, "/eb", MY_IP, strlen(MY_IP), &ZOO_OPEN_ACL_UNSAFE, 
            ZOO_PERSISTENT, buff, sizeof(buff));
        if (r != ZOK) printf("ERROR (%d): can't create /eb \n", r);

        for (i = 0; i < EB_CNT; i++) {
            path[0] = '\0';
            strcpy(path, "/eb/");
            strcat(path, EB_NODES[i]);
            r = zoo_create(zh, path, MY_IP, strlen(MY_IP), &ZOO_OPEN_ACL_UNSAFE, 
                ZOO_PERSISTENT, buff, sizeof(buff));
        }
    }
    else if (cluster_name = "mod") {
        r = zoo_create(zh, "/mod", MY_IP, strlen(MY_IP), &ZOO_OPEN_ACL_UNSAFE, 
            ZOO_PERSISTENT, buff, sizeof(buff));
        if (r != ZOK) printf("ERROR (%d): can't create /mod \n", r);

        for (i = 0; i < MOD_CNT; i++) {
            path[0] = '\0';
            strcpy(path, "/mod/");
            strcat(path, MOD_NODES[i]);
            r = zoo_create(zh, path, MY_IP, strlen(MY_IP), &ZOO_OPEN_ACL_UNSAFE, 
                ZOO_PERSISTENT, buff, sizeof(buff));
        }
    }
}

int main() 
{
    zoo_set_debug_level(ZOO_LOG_LEVEL_DEBUG);

    // initiating zookeeper connection
    zhandle_t *zkHandler = zookeeper_init("localhost:2181", NULL, 1000, 0, 0, 0);

    if (zkHandler == NULL) {
        printf("Failed to connect to Zookeeper\n");
        return errno;
    }
    printf("Connected sucessfully!\n");

    setHostName();
    setIPAdress();

    // creating /max znode

    char buff[256];
    int r = zoo_create(zkHandler, "/max", HOSTNAME, strlen(HOSTNAME),
        &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT, buff, sizeof(buff));

    if (r == ZOK) {
        // since this client managed to create /max znode, it gets to be the leader 

        printf("CREATED NODE (%s): leader initialized\n", buff);
        r = zoo_set(zkHandler, "/max", MY_IP, strlen(MY_IP), -1);
        strcpy(LEADER_IP, MY_IP);

        // creating /eb and client znodes (izdvojiti u posebnu funkciju)
        createCluster(zkHandler, "eb");
        printf("Leader created /eb\n");

        // creating /mod and client znodes (izdvojiti u posebnu funkciju)
        createCluster(zkHandler, "mod");
        printf("Leader created /mod\n");
    }
    else {
        // this client failed to create /max znode, therfore it is a worker 
        
        // check to which cluster does this worker belong
        if (checkCluster(zkHandler, "/eb")) {
            printf("%s is in /eb cluster\n", HOSTNAME);

            char path_to_node[15] = "/eb/";
            strcat(path_to_node, HOSTNAME);
            r = zoo_set(zkHandler, path_to_node, MY_IP, strlen(MY_IP), -1);
        }
        else if (checkCluster(zkHandler, "/mod")) {
            printf("%s is in /mod cluster\n", HOSTNAME);

            char path_to_node[15] = "/mod/";
            strcat(path_to_node, HOSTNAME);
            r = zoo_set(zkHandler, path_to_node, MY_IP, strlen(MY_IP), -1);
        }
        else {
            printf("ERROR: no cluster found!\n");
        }

        int len = sizeof(LEADER_IP);
        r = zoo_get(zkHandler, "/max", 0, LEADER_IP, &len, NULL);
        if (r != ZOK) printf("ERROR (%d): can't get leader ip \n", r);
    }

    printf("Leader is: %s\n", LEADER_IP);

    zookeeper_close(zkHandler);

    return 0;

}