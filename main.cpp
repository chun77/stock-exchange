#include "server.hpp"
#include "xmlSeqParser.hpp"
#include "dbController.hpp"
#include "xmlSeqGenerator.hpp"
#include <thread>

using namespace std;

int main() {
    server my_server;
    my_server.setupServer();
    cout << "Server setup complete. Waiting for connections..." << endl;
    dbController dbCtrler("exchange", "postgres", "passw0rd", "localhost", "5432");
    dbCtrler.initializeAccount();
    dbCtrler.initializePosition();
    dbCtrler.initializeOpened();
    dbCtrler.initializeCanceled();
    dbCtrler.initializeExecuted();

    while (true) {
        unsigned int client_socket_fd = my_server.acceptConnection();
        cout << "Connection accepted. Receiving message..." << endl;
        string xml_message = my_server.recvMessage(client_socket_fd);
        cout << "Received message from client: " << xml_message << endl;
        // 创建新线程处理请求
        thread request_thread(&server::handleRequest, my_server, xml_message); 

        // 在新线程上运行 handleRequest，处理请求
        request_thread.detach(); // 分离新线程，让其在后台运行
    }
    return 0;
}