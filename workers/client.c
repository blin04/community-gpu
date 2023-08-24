#include <stdio.h>
#include <errno.h>
#include <zookeeper/zookeeper.h>

static int connected = 0;
static int expired = 0;

struct zhandle_t *zkHandler;

void watcher(zhandle_t *zkH, int type, int state, const char *path, void *watcherCtx)
{

} 

int main()
{
    printf("Hey\n");
    zoo_set_debug_level(ZOO_LOG_LEVEL_DEBUG);

    zkHandler = zookeeper_init("localhost:2181", watcher, 1000, 0, 0, 0);

    if (!zkHandler) {
        return errno;
    }
    else {
        printf("Connection to Zookeeper established\n");
    }
    zookeeper_close(zkHandler);

    return 0;
}