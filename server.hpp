#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <unistd.h>
#include <string>

using namespace std;

class server {
private:
    const char* port;
    int socket_fd;

public:
    server();
    void setupServer();
    unsigned int acceptConnection();
    std::string recvMessage(int client_socket_fd);
    void sendMessage(int client_socket_fd, const string& message);
    void handleRequest(const string& xmlMsg, int client_socket_fd);
};

#endif
