#include <stdio.h>
#include <errno.h>
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

    struct String_vector s;
    int r = zoo_get_children(zkHandler, "/", 0, &s);

    printf("OUTPUT: %s \n", *s.data);

    zookeeper_close(zkHandler);

    return 0;

}