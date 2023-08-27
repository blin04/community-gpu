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

bool checkCluster(zhandle_t* zh, char* client_name, char* path) {
    /* checks if client is assigned to particular
        cluster identified with path */

    struct String_vector children;
    int r = zoo_get_children(zh, path, 0, &children);
    if (r != ZOK) {
        printf("ERROR: failed in checkCluster()");
        exit(1);
    }

    for (int i = 0; i < children.count; i++) {
        if (strcmp(client_name, children.data[i]) == 0) return true;
    }
    return false;
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
        r = zoo_create(zkHandler, "/eb", LEADER_IP, strlen(LEADER_IP),
            &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT, buff, sizeof(buff));
        if (r != ZOK)
            printf("ERROR! error code %d\n", r);

        r = zoo_create(zkHandler, "/eb/node2", "", strlen(""), 
            &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT, buff, sizeof(buff));
        if (r != ZOK)
            printf("ERROR! error code %d\n", r);

        printf("Leader created /eb\n");

        // creating /mod and client znodes (izdvojiti u posebnu funkciju)
        r = zoo_create(zkHandler, "/mod", LEADER_IP, strlen(LEADER_IP),
            &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT, buff, sizeof(buff));
        if (r != ZOK)
            printf("ERROR! error code %d\n", r);

        r = zoo_create(zkHandler, "/mod/node3", "", strlen(""), 
            &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT, buff, sizeof(buff));
        if (r != ZOK)
            printf("ERROR (%d): can't create znode \n", r);
        
        printf("Leader created /mod\n");

    }
    else {
        // this client failed to create /max znode, therfore it is a worker 
        
        if (r != ZOK) {
            // this client is in mod cluster

            if (checkCluster(zkHandler, HOSTNAME, "/eb")) {
                printf("%s is in /eb cluster\n", HOSTNAME);
            }
            else if (checkCluster(zkHandler, HOSTNAME, "/mod")) {
                printf("%s is in /mod cluster\n", HOSTNAME);
            }
            else {
                printf("ERROR: no cluster found!\n");
            }

            int len = sizeof(LEADER_IP);
            r = zoo_get(zkHandler, "/max", 0, LEADER_IP, &len, NULL);
            if (r != ZOK) printf("ERROR (%d): can't get leader ip \n", r);
        }
    }

    printf("Leader is: %s\n", LEADER_IP);

    zookeeper_close(zkHandler);

    return 0;

}