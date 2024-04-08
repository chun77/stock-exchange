#include "server.hpp"
#include "xmlSeqParser.hpp"
#include "dbController.hpp"
#include "xmlSeqGenerator.hpp"

using namespace std;

server::server(): port("12345"), socket_fd(-1) {}

void server::setupServer(){
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

unsigned int server::acceptConnection(){
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

string server::recvMessage(int client_socket_fd) {
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

void server::sendMessage(int client_socket_fd, const string& message){
    int bytes_sent = send(client_socket_fd, message.c_str(), message.size(), 0);
    if (bytes_sent == -1) {
        cerr << "Error: cannot send message to client" << endl;
        return;
    }
}

void server::handleRequest(const string& xmlMsg, int client_socket_fd){
    dbController dbCtrler("exchange", "postgres", "passw0rd", "localhost", "5432");
    xmlSeqParser parser;
    xmlSeqGenerator generator("results");
    int result = parser.parse(xmlMsg.c_str());
    if(result == 0){
        while ((result = parser.getNextCreate()) != -1){
            if(result == 0){
                pair<int, float> accountInfo = parser.getAccountInfo();
                createAccountResult car = dbCtrler.insertAccount(accountInfo.first, accountInfo.second);
                if(car.errMsg == ""){
                    generator.addElement(car.accountID);
                }else{
                    generator.addElement(car.accountID, car.errMsg);
                }
            } else if (result == 1){
                map<string, PairVec> symbolInfo = parser.getSymbolInfo();
                string symbol = symbolInfo.begin()->first;
                PairVec actAndNum = symbolInfo.begin()->second;
                for(const auto& pair: actAndNum){
                    createSymResult csr = dbCtrler.insertSymbol(symbol, pair.first, pair.second);
                    if(csr.errMsg == ""){
                        generator.addElement(csr.symbol, csr.accountID);
                    } else {
                        generator.addElement(csr.symbol, csr.accountID, csr.errMsg);
                    }
                }
            }
        }
    } else if(result == 1){
        while((result = parser.getNextTrans()) != -1){
            if(result == 0){
                int accountID = parser.getAccountIdForTrans();
                tuple<string, float, float> orderInfo = parser.getOrderInfo();
                transOrderResult tor = dbCtrler.insertOpened(accountID, get<0>(orderInfo), get<1>(orderInfo), get<2>(orderInfo));
                if(tor.errMsg == ""){
                    generator.addElement(tor.symbol, tor.amount, tor.limit, tor.transID);
                } else {
                    generator.addElement(tor.symbol, tor.amount, tor.limit, tor.errMsg);
                }
            } else if (result == 1){
                int accountID = parser.getAccountIdForTrans();
                int queryID = parser.getQueryID();
                transQueryResult tqr = dbCtrler.queryShares(accountID, queryID);
                if(tqr.errMsg == ""){
                    generator.addElement(tqr.transID, tqr.openedShares, tqr.canceledShares, tqr.cancelTime, tqr.executedShares);
                } else {
                    generator.addElement(tqr.transID, tqr.openedShares, tqr.canceledShares, tqr.cancelTime, tqr.executedShares, tqr.errMsg);
                }
            } else if (result == 2){
                int accountID = parser.getAccountIdForTrans();
                int cancelID = parser.getCancelID();
                transCancelResult tcr = dbCtrler.insertCanceled(accountID, cancelID);
                if(tcr.errMsg == ""){
                    generator.addElement(tcr.transID, tcr.canceledShares, tcr.cancelTime, tcr.executedShares);
                } else {
                    generator.addElement(tcr.transID, tcr.canceledShares, tcr.cancelTime, tcr.executedShares, tcr.errMsg);
                }
            }
        }
    }

    string xmlResponse = generator.getXML();
    cout << xmlResponse << endl;
    sendMessage(client_socket_fd, xmlResponse);
}