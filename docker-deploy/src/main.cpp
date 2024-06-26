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
    while(true){
        try{
            dbController dbCtrler("exchange", "postgres", "passw0rd", "db", "5432");
            dbCtrler.initializeAccount();
            dbCtrler.initializePosition();
            dbCtrler.initializeOpened();
            dbCtrler.initializeCanceled();
            dbCtrler.initializeExecuted();
            break;
        } catch (const exception& e) {
            continue;
        }
        
    }
    
    while (true) {
        unsigned int client_socket_fd = my_server.acceptConnection();
        //cout << "Connection accepted. Receiving message..." << endl;
        string xml_message = my_server.recvMessage(client_socket_fd);
        //cout << "Received message from client: " << xml_message << endl;

        thread request_thread(&server::handleRequest, my_server, xml_message, client_socket_fd); 
        //request_thread.join();
        request_thread.detach(); 
    }
    return 0;
}