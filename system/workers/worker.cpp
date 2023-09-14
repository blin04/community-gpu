#include <iostream>
#include <string>

#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <zookeeper/zookeeper.h>

using namespace std;

string LEADER_IP = "";
int LEADER_PORT = 11000;

int main()
{
    zoo_set_debug_level(ZOO_LOG_LEVEL_DEBUG);

    // initiating zookeeper connection
    zhandle_t *zkHandler = zookeeper_init("localhost:2181", NULL, 1000, 0, 0, 0);

    if (zkHandler == NULL) {
        cout << "Failed to connect to Zookeeper\n";
        exit(EXIT_FAILURE);
    }
    cout << "Connected sucessfully!\n";

    // get IP address of cluster leader
    int len = sizeof(LEADER_IP);
    char ip_buff[len] = "";
    int r = zoo_get(zkHandler, "/eb/node2", 0, ip_buff, &len, NULL);
    if (r != ZOK) {
        cout << "ERROR (" << r << "): can't get leader ip \n";
        exit(EXIT_FAILURE);
    }
    LEADER_IP = ip_buff;
    cout << "Cluster Leader IP: " << LEADER_IP << "\n";

    // open socket for communication with cluster leader
    int leader_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (leader_socket < 0) {
        cout << "ERROR (" << errno << "): can't create socket\n";
        exit(EXIT_FAILURE);
    }

    // struct containing address of cluster leader
    sockaddr_in leader_addr;
    leader_addr.sin_family = AF_INET;
    leader_addr.sin_port = htons(LEADER_PORT);

    // convert IPv4 address of leader
    // from text to binary form
    r = inet_pton(AF_INET, &LEADER_IP[0], &leader_addr.sin_addr);
    if (r <= 0) {
        cout << "ERROR (" << r << "): invalid address\n";
        exit(EXIT_FAILURE);
    } 

    // connect to cluster leader
    r = connect(leader_socket, (sockaddr*)&leader_addr, sizeof(leader_addr));
    if (r < 0) {
        cout << "ERROR (" << strerror(errno) << "): can't connect\n";
    }
    cout << "Connection with sever sucessful!\n";

    return 0;
}