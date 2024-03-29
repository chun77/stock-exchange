#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>

using namespace std;

int main() {
    int client_socket_fd;
    struct sockaddr_in server_addr;

    client_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket_fd == -1) {
        cerr << "Error: cannot create socket" << endl;
        return 1;
    }

    // 设置服务器地址信息
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345); // 设置服务器端口
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr); // 设置服务器 IP 地址

    // 连接到服务器
    if (connect(client_socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        cerr << "Error: cannot connect to server" << endl;
        close(client_socket_fd);
        return 1;
    }

    // 发送消息给服务器
    const char * message = R"(429
        <create>
            <account id="1" balance="100.5"/>
            <account id="2" balance="200.5"/>
            <symbol sym="SYM1">
                <account id="1">50.5</account>
                <account id="2">60.5</account>
            </symbol>
            <symbol sym="SYM2">
                <account id="1">70.5</account>
                <account id="2">80.5</account>
            </symbol>
        </create>
    )";
    int bytes_sent = send(client_socket_fd, message, strlen(message), 0);
    if (bytes_sent == -1) {
        cerr << "Error: cannot send message to server" << endl;
        close(client_socket_fd);
        return 1;
    }

    cout << "Message sent to server: " << message << endl;

    // 关闭套接字
    close(client_socket_fd);

    return 0;
}
