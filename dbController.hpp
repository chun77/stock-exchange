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

struct transOrderResult{
    string symbol;
    float amount;
    float limit;
    int transID;
    string errMsg;
};

struct transCancelResult{
    int transID;
    float canceledShares;
    time_t cancelTime;
    vector<tuple<float, float, time_t>> executedShares;
    string errMsg;
};

struct transQueryResult{
    int transID;
    float openedShares;
    float canceledShares;
    time_t cancelTime;
    vector<tuple<float, float, time_t>> executedShares;
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
    void initializeOpened();
    void initializeCanceled();
    void initializeExecuted();
    createAccountResult insertAccount(int accountID, float balance);
    createSymResult insertSymbol(string symbol, int accountID, float NUM);
    transOrderResult insertOpened(int accountID, string symbol, float amt, float limit);
    transCancelResult insertCanceled(int transID);

    transQueryResult queryShares(int transID);


};

#endif