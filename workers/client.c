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

    printf("----------- get_children ------------\n");
    struct String_vector s;
    int r = zoo_get_children(zkHandler, "/", 0, &s);

    printf("OUTPUT: %s \n", *s.data);

    printf("----------- get ------------\n");
    char buff[512];
    int len = sizeof(buff);
    r = zoo_get(zkHandler, "/test_node", 0, buff, &len, NULL);

    printf("RETURED WITH %d\n", r);
    printf("DATA: %s\n", buff);

    printf("----------- set ------------\n");

    strcpy(buff, "client");
    r = zoo_set(zkHandler, "/test_node", buff, len, -1); // proveriti version

    zookeeper_close(zkHandler);

    return 0;

}