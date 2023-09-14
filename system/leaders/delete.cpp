/*

This program is used to delete ZooKeeper tree that is
built during calculations. This should be added to main 
program once development finishes, so that it 'cleans' 
ZooKeeper tree after it finishes. 

*/

#include <iostream>
#include <string>

#include <zookeeper/zookeeper.h>

using namespace std;

// size of eb cluster and nodes assigned to that cluster
int EB_CNT = 2;
string EB_NODES[] = {"node2", "node4"};

// size of mod cluster and nodes assigned to that cluster
int MOD_CNT = 1;
string MOD_NODES[] = {"node3"};

int main()
{
    zhandle_t *zkHandler = zookeeper_init("localhost:2181", NULL, 1000, 0, 0, 0);
    if (zkHandler == NULL) {
        cout << "Failed to connect to Zookeeper\n";
        exit(EXIT_FAILURE);
    }
    cout << "Connected sucessfully!\n";

    // deleting /max znode
    int r = zoo_delete(zkHandler, "/max", -1);
    if (r != ZOK) {
        cout << "Failed to delete /max\n";
        exit(EXIT_FAILURE);
    }

    string path;
    // deleting worker znodes from /eb cluster
    for (int i = 0; i < EB_CNT; i++) {
        path = "/eb/" + EB_NODES[i];
        r = zoo_delete(zkHandler, &path[0], -1);
        if (r != ZOK) {
            cout << "Failed to delete /eb nodes \n";
            exit(EXIT_FAILURE);
        }
    }

    // deleting worker znodes from /mod cluster
    for (int i = 0; i < MOD_CNT; i++) {
        path = "/mod/" + MOD_NODES[i];
        r = zoo_delete(zkHandler, &path[0], -1);
        if (r != ZOK) {
            cout << "Failed to delete /mod nodes \n";
            exit(EXIT_FAILURE);
        }
    }

    // deleting /eb znode
    r = zoo_delete(zkHandler, "/eb", -1);
    if (r != ZOK) {
        cout << "Failed to delete /eb\n";
        exit(EXIT_FAILURE);
    }

    // deleting /mod znode
    r = zoo_delete(zkHandler, "/mod", -1);
    if (r != ZOK) {
        cout << "Failed to delete /mod\n";
        exit(EXIT_FAILURE);
    }

    cout << "Sucessfully deleted zNodes\n";

    return 0;
}