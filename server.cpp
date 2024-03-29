#include <iostream>
#include <sys/socket.h> // bind, listen, accept...
#include <netinet/in.h> // struct sockaddr_in
#include <arpa/inet.h> // inet_addr(), inet_ntoa()
#include <netdb.h> // getaddrinfo(), getnameinfo()
#include <cstring>
#include <unistd.h>

using namespace std;

class server{
private:
    const char* port;
    int socket_fd;

public:

    server(): port("12345"), socket_fd(-1) {}

    void setupServer(){
        struct addrinfo host_info;
        struct addrinfo * host_info_list;
        int status;

        memset(&host_info, 0, sizeof(host_info));
        host_info.ai_family   = AF_UNSPEC;
        host_info.ai_socktype = SOCK_STREAM;
        host_info.ai_flags    = AI_PASSIVE;
        const char * hostname = INADDR_ANY;

        status = getaddrinfo(hostname, port, & host_info, & host_info_list);
        if (status != 0) {
            cerr << "Error: cannot get address info for host" << endl;
            cerr << "  (" << hostname << "," << port << ")" << endl;
            exit(-1);
        }

        socket_fd = socket(host_info_list -> ai_family,
                        host_info_list -> ai_socktype,
                        host_info_list -> ai_protocol);
        if (socket_fd == -1) {
            cerr << "Error: cannot create socket" << endl;
            exit(-1);
        }

        int yes = 1;
        status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
        if (status == -1) {
            cerr << "Error: cannot bind socket" << endl;
            exit(-1);
        }

        status = listen(socket_fd, 128);
        if (status == -1) {
            cerr << "Error: cannot listen on socket" << endl;
            exit(-1);
        }

        freeaddrinfo(host_info_list);
    }

    unsigned int acceptConnection(){
        int client_socket_fd;
        struct sockaddr_storage socket_addr;
        socklen_t socket_addr_len = sizeof(socket_addr);
        client_socket_fd = accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
        if (client_socket_fd == -1) {
            std::cerr << "Error: cannot accept connection on socket" << std::endl;
            exit(-1);
        }
        return client_socket_fd;
    }

    string recvMessage(int client_socket_fd) {
        const int BUFFER_SIZE = 4096;
        char buffer[BUFFER_SIZE];

        int bytes_received = recv(client_socket_fd, buffer, BUFFER_SIZE, 0);
        if (bytes_received == -1) {
            cerr << "Error: cannot receive message from client" << endl;
            exit(-1);
        }

        string message(buffer, bytes_received);

        size_t newline_pos = message.find('\n');
        if (newline_pos == string::npos) {
            cerr << "Error: invalid message format from client" << endl;
            exit(-1);
        }
        string length_str = message.substr(0, newline_pos);
        unsigned int length = stoi(length_str);

        string xml_message = message.substr(newline_pos + 1, length);

        return xml_message;
    }

};

int main() {
    server my_server;
    my_server.setupServer();

    cout << "Server setup complete. Waiting for connections..." << endl;

    while (true) {
        unsigned int client_socket_fd = my_server.acceptConnection();
        cout << "Connection accepted. Receiving message..." << endl;

        string xml_message = my_server.recvMessage(client_socket_fd);
        cout << "Received message from client: " << xml_message << endl;

        close(client_socket_fd);
        cout << "Connection closed." << endl;
    }

    return 0;
}
