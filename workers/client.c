#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <zookeeper/zookeeper.h>

#define HOSTNAME_LEN 100

char HOSTNAME[HOSTNAME_LEN];

int main() 
{
    zoo_set_debug_level(ZOO_LOG_LEVEL_DEBUG);

    zhandle_t *zkHandler = zookeeper_init("localhost:2181", NULL, 1000, 0, 0, 0);

    if (zkHandler == NULL) {
        printf("Failed to connect to Zookeeper\n");
        return errno;
    }
    printf("Connected sucessfully!\n");

    int hostname = gethostname(HOSTNAME, HOSTNAME_LEN);

    if (hostname == -1) {
        perror("ERROR: failed in gethostname()");
        exit(1);
    }

    char buff[256];
    struct ACL_vector acl = ZOO_OPEN_ACL_UNSAFE;
    int r = zoo_create(zkHandler, "/max", HOSTNAME, strlen(HOSTNAME),
        &acl, ZOO_PERSISTENT, buff, sizeof(buff));

    if (r == ZOK) {
        printf("CREATED NODE (%s): leader initialized\n", buff);
    }
    else {
        printf("ERROR (%d): couldn't create node, %s is a worker \n", r, HOSTNAME);
        return errno;
    }

    zookeeper_close(zkHandler);

    return 0;

}