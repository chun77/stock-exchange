#ifndef DBCONTROLLER_HPP
#define DBCONTROLLER_HPP

#include "xmlSeqGenerator.hpp"
#include "xmlSeqParser.hpp"
#include <pqxx/pqxx>

using namespace std;
using namespace pqxx;

struct createAccountResult{
    int accountID;
    string errMsg;
};

struct createSymResult{
    string symbol;
    int accountID;
    string errMsg;
};

class dbController {
private:
    connection con;
public:
    dbController(const std::string& dbName, const std::string& user, const std::string& password, const std::string& host, const std::string& port)
        : con("dbname=" + dbName + " user=" + user + " password=" + password + " host=" + host + " port=" + port) {
    }

    void initializeAccount();
    void initializePosition();
    createAccountResult insertAccount(int accountID, float balance);
    createSymResult insertSymbol(string symbol, int accountID, float NUM);
};

#endif