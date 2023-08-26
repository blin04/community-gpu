#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <zookeeper/zookeeper.h>

#define HOSTNAME_LEN 100

// globals for storing host device name and IP address of leader
char HOSTNAME[HOSTNAME_LEN];
char LEADER_IP[32] = "";

char* getIPAdress() {
    /* funtion for getting host machine's IP address*/

    struct hostent* host_entry = gethostbyname(HOSTNAME);

    if (host_entry == NULL) {
        perror("ERROR: failed in getIPAdress()\n");
        exit(1);
    }

    char* IPbuffer = inet_ntoa(*(struct in_addr*)
                                    host_entry->h_addr_list[0]);
    return IPbuffer;
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

    // geting device hostname
    int hostname = gethostname(HOSTNAME, HOSTNAME_LEN);

    if (hostname == -1) {
        perror("ERROR: failed in gethostname()\n");
        exit(1);
    }

    /* creating znode
        buff - used for storing path of znode 
        ZOO_PERSISTENT - flag which stands for creating znode
        ZO_OPEN_ACL_UNSAFE - acl with scheme ('world':'anyone')
    */
    char buff[256];
    struct ACL_vector acl = ZOO_OPEN_ACL_UNSAFE;
    int r = zoo_create(zkHandler, "/max", HOSTNAME, strlen(HOSTNAME),
        &acl, ZOO_PERSISTENT, buff, sizeof(buff));

    char* my_ip;
    if (r == ZOK) {
        // since this client managed to create znode, it gets to be the leader 

        printf("CREATED NODE (%s): leader initialized\n", buff);

        my_ip = getIPAdress();
        r = zoo_set(zkHandler, "/max", my_ip, strlen(my_ip), -1);

        strcpy(LEADER_IP, my_ip);
    }
    else {
        // thisclient failed to create znode, therfore it is a worker 
        
        printf("ERROR (%d): couldn't create node, %s is a worker \n", r, HOSTNAME);

        int len = sizeof(LEADER_IP);
        zoo_get(zkHandler, "/max", 0, LEADER_IP, &len, NULL);
    }

    printf("Leader is: %s\n", LEADER_IP);

    zookeeper_close(zkHandler);

    return 0;

}