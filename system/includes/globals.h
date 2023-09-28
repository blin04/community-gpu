/*
    This file contains some global variables that
    are used by both leaders and workers
*/

#include <string>

std::string NODES_PATH = "/graph/nodes";       // path to file containing nodes
std::string EDGES_PATH = "/graph/edges";       // path to file containing edges

// size of eb cluster and nodes assigned to that cluster (first node specified becomes cluster leader)
int EB_CNT = 2;
std::string EB_NODES[] = {"node2", "node4"};

// size of mod cluster and nodes assigned to that cluster (first node specified becomes cluster leader)
int MOD_CNT = 2;
std::string MOD_NODES[] = {"node3", "node5"};
