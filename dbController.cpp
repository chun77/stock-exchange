#include "dbController.hpp"

createAccountResult dbController::insertAccount(int accountID, float balance){
    createAccountResult result;
    try {
        pqxx::work txn(con);
        std::string sql = "INSERT INTO Account (accountID, balance) VALUES (" + std::to_string(accountID) + ", " + std::to_string(balance) + ")";
        txn.exec(sql);
        txn.commit();
        result.accountID = accountID;
    } catch (const std::exception& e) {
        result.accountID = accountID;
        result.errMsg = e.what();
    }
    return result;
}

createSymResult dbController::insertSymbol(string symbol, int accountID, float NUM){
    createSymResult result;
    result.accountID = accountID;
    result.symbol = symbol;
    // Check if accountID exists in Account table
    pqxx::work txn(con);
    pqxx::result res = txn.exec("SELECT COUNT(*) FROM Account WHERE accountID = " + txn.quote(accountID));
    int count = res[0][0].as<int>();
    if (count == 0) {
        result.errMsg = "This accountID doesn't exist";
        return result;
    }
    // Check if symbol exists in Position table for the given accountID
    res = txn.exec("SELECT COUNT(*) FROM Position WHERE symbol = " + txn.quote(symbol) + " AND accountID = " + txn.quote(accountID));
    count = res[0][0].as<int>();
    if (count == 0) {
        // Insert new entry
        txn.exec("INSERT INTO Position (symbol, accountID, NUM) VALUES (" + txn.quote(symbol) + ", " + txn.quote(accountID) + ", " + txn.quote(NUM) + ")");
        txn.commit();
        return result;
    } else {
        // Update existing entry directly in SQL query
        txn.exec("UPDATE Position SET NUM = NUM + " + txn.quote(NUM) + " WHERE symbol = " + txn.quote(symbol) + " AND accountID = " + txn.quote(accountID));
        txn.commit();
        return result;
    }
}

/*transOrderResult dbController::insertOpened(int accountID, string symbol, float amt, float limit){
    transOrderResult result;
    result.symbol = symbol;
    result.limit = limit;
    result.amount = amt;
    
}*/

void dbController::initializeAccount() {
    try {
        pqxx::work txn(con);
        // Create Account table if it doesn't exist
        txn.exec("CREATE TABLE IF NOT EXISTS Account ("
                 "accountID INT PRIMARY KEY,"
                 "balance FLOAT"
                 ")");
        txn.commit();
        std::cout << "Account initialized successfully." << std::endl;
    } catch (const exception& e) {
        std::cerr << "Error initializing account: " << e.what() << std::endl;
    }
}

void dbController::initializePosition() {
    try{
        pqxx::work txn(con);
        txn.exec("CREATE TABLE IF NOT EXISTS Position ("
                    "symbol VARCHAR(255),"
                    "accountID INT,"
                    "NUM FLOAT,"
                    "FOREIGN KEY (accountID) REFERENCES Account(accountID),"
                    "PRIMARY KEY (symbol, accountID)"
                    ")");
        txn.commit();
        std::cout << "Position initialized successfully." << std::endl;
    } catch (const exception& e) {
        std::cerr << "Error initializing position: " << e.what() << std::endl;
    }
}

void dbController::initializeOpened(){
    try{
        pqxx::work txn(con);
        txn.exec(
            "CREATE TABLE IF NOT EXISTS OpenOrder ("
            "   transID SERIAL PRIMARY KEY,"
            "   accountID INT REFERENCES Account(accountID),"
            "   symbol VARCHAR(255),"
            "   amt FLOAT,"
            "   limit FLOAT"
            ")"
        );
        txn.commit();
        std::cout << "Opened initialized successfully." << std::endl;
    } catch (const exception& e) {
        std::cerr << "Error initializing opened: " << e.what() << std::endl;
    }
}

void dbController::initializeCanceled(){
    try {
        pqxx::work txn(con);
        txn.exec(
            "CREATE TABLE IF NOT EXISTS OpenOrder ("
            "   transID SERIAL PRIMARY KEY,"
            "   accountID INT REFERENCES Account(accountID),"
            "   symbol VARCHAR(255),"
            "   amt FLOAT,"
            "   time TIMESTAMP"
            ")"
        );
        txn.commit();
        cout << "Canceled table initialized successfully." << endl;
    } catch (const exception& e) {
        cerr << "Error initializing canceled: " << e.what() << endl;
    }
}

void dbController::initializeExecuted(){
    try {
        pqxx::work txn(con);
        txn.exec(
            "CREATE TABLE IF NOT EXISTS OpenOrder ("
            "   transID SERIAL PRIMARY KEY,"
            "   accountID INT REFERENCES Account(accountID),"
            "   symbol VARCHAR(255),"
            "   price FLOAT,"
            "   amt FLOAT,"
            "   time TIMESTAMP"
            ")"
        );
        txn.commit();
        cout << "Executed table initialized successfully." << endl;
    } catch (const exception& e) {
        cerr << "Error initializing executed: " << e.what() << endl;
    }
}


int main(){
    // PostgreSQL connection parameters
    std::string dbName = "exchange";
    std::string user = "postgres"; 
    std::string password = "passw0rd";
    std::string host = "localhost"; 
    std::string port = "5432"; 
    // Create an instance of dbController
    dbController db(dbName, user, password, host, port);
    // Initialize the database (create tables if necessary)
    db.initializeAccount();
    // Insert a new account
    int accountID = 123;
    float balance = 1000.0;
    createAccountResult result = db.insertAccount(accountID, balance);
    // Handle the result
    if (result.errMsg == "") {
        std::cout << "Successfully inserted account with ID: " << result.accountID << std::endl;
    } else {
        std::cerr << "Failed to insert account with ID: " << result.accountID << ". Error: " << result.errMsg << std::endl;
    }
    accountID = 123;
    balance = 1000.0;
    result = db.insertAccount(accountID, balance);
    // Handle the result
    if (result.errMsg == "") {
        std::cout << "Successfully inserted account with ID: " << result.accountID << std::endl;
    } else {
        std::cerr << "Failed to insert account with ID: " << result.accountID << ". Error: " << result.errMsg << std::endl;
    }
    accountID = 124;
    balance = 1000.0;
    result = db.insertAccount(accountID, balance);
    // Handle the result
    if (result.errMsg == "") {
        std::cout << "Successfully inserted account with ID: " << result.accountID << std::endl;
    } else {
        std::cerr << "Failed to insert account with ID: " << result.accountID << ". Error: " << result.errMsg << std::endl;
    }

    // 初始化 Position 表
    db.initializePosition();
    createSymResult sResult;
    // 插入一些测试数据
    sResult = db.insertSymbol("AAPL", 123, 100);
    if(sResult.errMsg == ""){
        cout << "Successfully inserted symbol with ID: " << sResult.accountID << " and symbol: " << sResult.symbol << endl;
    } else {
        std::cerr << sResult.errMsg << endl;
    }
    sResult = db.insertSymbol("GOOGL", 123, 50);
    if(sResult.errMsg == ""){
        cout << "Successfully inserted symbol with ID: " << sResult.accountID << " and symbol: " << sResult.symbol << endl;
    } else {
        std::cerr << sResult.errMsg << endl;
    }
    sResult = db.insertSymbol("MSFT", 54321, 75);
    if(sResult.errMsg == ""){
        cout << "Successfully inserted symbol with ID: " << sResult.accountID << " and symbol: " << sResult.symbol << endl;
    } else {
        std::cerr << sResult.errMsg << endl;
    }

    return 0;
}
