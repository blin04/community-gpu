#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <zookeeper/zookeeper.h>

int main()
{
    zoo_set_debug_level(ZOO_LOG_LEVEL_DEBUG);

    zhandle_t *zkHandler = zookeeper_init("localhost:2181", NULL, 1000, 0, 0, 0);

    if (zkHandler == NULL) {
        printf("Failed to connect to Zookeeper\n");
        return errno;
    }

    printf("Connected sucessfully!\n");

    char buff[256];
    struct ACL_vector acl = ZOO_OPEN_ACL_UNSAFE;
    int r = zoo_create(zkHandler, "/my_node", "client", 6,
        &acl, ZOO_PERSISTENT, buff, sizeof(buff));

    if (r == ZOK) {
        printf("CREATED NODE: %s", buff);
    }
    else {
        printf("ERROR: %d", r);
        return errno;
    }

    zookeeper_close(zkHandler);

    return 0;

}