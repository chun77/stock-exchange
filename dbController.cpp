#include "dbController.hpp"

createAccountResult dbController::insertAccount(int accountID, float balance){
    createAccountResult result;
    result.accountID = accountID;
    try {
        pqxx::work txn(con);
        std::string sql = "INSERT INTO Account (accountID, balance) VALUES (" + std::to_string(accountID) + ", " + std::to_string(balance) + ")";
        txn.exec(sql);
        txn.commit();
    } catch (const std::exception& e) {
        result.errMsg = e.what();
    }
    return result;
}

createSymResult dbController::insertSymbol(string symbol, int accountID, float NUM){
    createSymResult result;
    result.accountID = accountID;
    result.symbol = symbol;
    try{
        // Check if accountID exists in Account table
        pqxx::work txn(con);
        pqxx::result res = txn.exec("SELECT COUNT(*) FROM Account WHERE accountID = " + txn.quote(accountID));
        int count = res[0][0].as<int>();
        if (count == 0) {
            result.errMsg = "This accountID doesn't exist";
            return result;
        }
        // Insert new entry
        txn.exec("INSERT INTO Position (symbol, accountID, NUM) VALUES (" 
        + txn.quote(symbol) + ", " + txn.quote(accountID) + ", " + txn.quote(NUM) + ") "
        + "ON CONFLICT (symbol, accountID) DO UPDATE SET NUM = Position.NUM + " + txn.quote(NUM)
        + " WHERE Position.symbol = " + txn.quote(symbol) + " AND Position.accountID = " + txn.quote(accountID));
        txn.commit();
    } catch (const exception& e) {
        result.errMsg = e.what();
    }
    return result;
}

transOrderResult dbController::insertOpened(int accountID, string symbol, float amt, float limit){
    transOrderResult result;
    result.symbol = symbol;
    result.limit = limit;
    result.amount = amt;
    try {
        pqxx::work txn(con);
        // Check if accountID exists in Account table
        pqxx::result res = txn.exec("SELECT COUNT(*) FROM Account WHERE accountID = " + txn.quote(accountID));
        int count = res[0][0].as<int>();
        if (count == 0) {
            result.errMsg = "This accountID doesn't exist";
            return result;
        }
        pqxx::result transIDRes;
        if (amt > 0) {
            // Buying operation
            float requiredBalance = amt * limit;
            res = txn.exec("SELECT balance FROM Account WHERE accountID = " + txn.quote(accountID) + "FOR UPDATE");
            float balance = res[0]["balance"].as<float>();
            if (balance < requiredBalance) {
                result.errMsg = "Insufficient balance";
                return result;
            }
            // Deduct from balance
            float newBalance = balance - requiredBalance;
            txn.exec("UPDATE Account SET balance = " + txn.quote(newBalance) + " WHERE accountID = " + txn.quote(accountID));
            // Insert into OpenedOrder table
            transIDRes = txn.exec("INSERT INTO OpenedOrder (accountID, symbol, amt, price_limit, time) VALUES (" + txn.quote(accountID) + ", " + txn.quote(symbol) + ", " + txn.quote(amt) + ", " + txn.quote(limit) + ", " + txn.quote(time(NULL)) + ") RETURNING transID");
        } else {
            // Selling operation
            res = txn.exec("SELECT NUM FROM Position WHERE accountID = " + txn.quote(accountID) + " AND symbol = " + txn.quote(symbol) + "FOR UPDATE");
            if (res.size() == 0) {
                result.errMsg = "No position found for the symbol";
                return result;
            }
            float currentNUM = res[0]["NUM"].as<float>();
            if (currentNUM < std::abs(amt)) {
                result.errMsg = "Insufficient shares";
                return result;
            }
            // Deduct shares from position
            float newNUM = currentNUM - std::abs(amt);
            txn.exec("UPDATE Position SET NUM = " + txn.quote(newNUM) + " WHERE accountID = " + txn.quote(accountID) + " AND symbol = " + txn.quote(symbol));
            // Insert into OpenedOrder table
            transIDRes = txn.exec("INSERT INTO OpenedOrder (accountID, symbol, amt, price_limit, time) VALUES (" + txn.quote(accountID) + ", " + txn.quote(symbol) + ", " + txn.quote(amt) + ", " + txn.quote(limit) + ", " + txn.quote(time(NULL)) + ") RETURNING transID");
        }
        // Commit transaction
        txn.commit();
        // Set result transID
        result.transID = transIDRes[0][0].as<int>();
    } catch (const std::exception& e) {
        result.errMsg = e.what();
    }
    return result;
}

transCancelResult dbController::insertCanceled(int transID){
    transCancelResult result;
    result.transID = transID;
    try {
        // Check if transID exists in OpenedOrder table
        pqxx::work txn(con);
        pqxx::result res = txn.exec("SELECT * FROM OpenedOrder WHERE transID = " + txn.quote(transID) + "FOR UPDATE");
        if (res.empty()) {
            result.errMsg = "No transaction found with transID: " + std::to_string(transID) + "\n";
            return result;
        }
        // Get details of the transaction
        float canceledShares;
        time_t cancelTime = time(nullptr);
        canceledShares = res[0]["amt"].as<float>();
        // Insert into CanceledOrder table
        txn.exec("INSERT INTO CanceledOrder (transID, accountID, symbol, amt, time) SELECT transID, accountID, symbol, amt, " + txn.quote(cancelTime) + " FROM OpenedOrder WHERE transID = " + txn.quote(transID));
        // Delete from OpenedOrder table
        txn.exec("DELETE FROM OpenedOrder WHERE transID = " + txn.quote(transID));
        result.canceledShares = canceledShares;
        result.cancelTime = cancelTime;
        pqxx::result executedRes = txn.exec(
            "SELECT amt, price, time FROM ExecutedOrder WHERE transID = " + txn.quote(transID)
        );
        for (const auto& row : executedRes) {
            float amt = row["amt"].as<float>();
            float price = row["price"].as<float>();
            time_t t = row["time"].as<time_t>();
            result.executedShares.push_back(std::make_tuple(amt, price, t));
        }
        txn.commit();
    } catch (const std::exception& e) {
        result.errMsg = e.what();
    }
    return result;
}

transQueryResult dbController::queryShares(int transID){
    transQueryResult result;
    result.transID = transID;
    result.openedShares = 0;
    result.canceledShares = 0;
    result.executedShares = {};
    result.errMsg = "";
    result.cancelTime = time(NULL);
    try {
        pqxx::work txn(con, "repeatable read");
        pqxx::result openedRes = txn.exec(
            "SELECT amt FROM OpenedOrder WHERE transID = " + txn.quote(transID)
        );
        pqxx::result canceledRes = txn.exec(
            "SELECT amt, time FROM CanceledOrder WHERE transID = " + txn.quote(transID)
        );
        pqxx::result executedRes = txn.exec(
            "SELECT amt, price, time FROM ExecutedOrder WHERE transID = " + txn.quote(transID)
        );
        if(openedRes.empty() && canceledRes.empty() && executedRes.empty()){
            result.errMsg = "Invalid trans_ID\n";
            return result;
        }
        if (!openedRes.empty()) {
            result.openedShares = openedRes[0]["amt"].as<float>();
        }
        if (!canceledRes.empty()) {
            result.canceledShares = canceledRes[0]["amt"].as<float>();
            result.cancelTime = canceledRes[0]["time"].as<time_t>();
        }
        if (!executedRes.empty()){
            for (const auto& row : executedRes) {
                float amt = row["amt"].as<float>();
                float price = row["price"].as<float>();
                time_t t = row["time"].as<time_t>();
                result.executedShares.push_back(std::make_tuple(amt, price, t));
            }
        }
        txn.commit();
    } catch (const std::exception& e) {
        std::cerr << "Error querying shares: " << e.what() << std::endl;
    }
    return result;
}

void dbController::initializeAccount() {
    try {
        pqxx::work txn(con);
        // Drop Account table if it exists
        txn.exec("DROP TABLE IF EXISTS Account CASCADE");
        
        // Create Account table
        txn.exec("CREATE TABLE Account ("
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
        txn.exec("DROP TABLE IF EXISTS Position");
        txn.exec("CREATE TABLE Position ("
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
        txn.exec("DROP TABLE IF EXISTS OpenedOrder");
        txn.exec(
            "CREATE TABLE OpenedOrder ("
            "   transID SERIAL PRIMARY KEY,"
            "   accountID INT REFERENCES Account(accountID),"
            "   symbol VARCHAR(255),"
            "   amt FLOAT,"
            "   price_limit FLOAT,"
            "   time BIGINT"
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
        txn.exec("DROP TABLE IF EXISTS CanceledOrder");
        txn.exec(
            "CREATE TABLE CanceledOrder ("
            "   transID SERIAL PRIMARY KEY,"
            "   accountID INT REFERENCES Account(accountID),"
            "   symbol VARCHAR(255),"
            "   amt FLOAT,"
            "   time BIGINT"
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
        txn.exec("DROP TABLE IF EXISTS ExecutedOrder");
        txn.exec(
            "CREATE TABLE ExecutedOrder ("
            "   transID SERIAL PRIMARY KEY,"
            "   accountID INT REFERENCES Account(accountID),"
            "   symbol VARCHAR(255),"
            "   price FLOAT,"
            "   amt FLOAT,"
            "   time BIGINT"
            ")"
        );
        txn.commit();
        cout << "Executed table initialized successfully." << endl;
    } catch (const exception& e) {
        cerr << "Error initializing executed: " << e.what() << endl;
    }
}

void test_insertAccount(dbController& db){
    createAccountResult result = db.insertAccount(123, 1000.0);
    // Handle the result
    if (result.errMsg == "") {
        std::cout << "Successfully inserted account with ID: " << result.accountID << std::endl;
    } else {
        std::cerr << "Failed to insert account with ID: " << result.accountID << ". Error: " << result.errMsg << std::endl;
    }
    result = db.insertAccount(123, 1200);
    // Handle the result
    if (result.errMsg == "") {
        std::cout << "Successfully inserted account with ID: " << result.accountID << std::endl;
    } else {
        std::cerr << "Failed to insert account with ID: " << result.accountID << ". Error: " << result.errMsg << std::endl;
    }
    result = db.insertAccount(1234, 1300);
    // Handle the result
    if (result.errMsg == "") {
        std::cout << "Successfully inserted account with ID: " << result.accountID << std::endl;
    } else {
        std::cerr << "Failed to insert account with ID: " << result.accountID << ". Error: " << result.errMsg << std::endl;
    }
}

void test_insertSymbol(dbController& db){
    createSymResult sResult;
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
    sResult = db.insertSymbol("GOOGL", 124, 50);
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
    sResult = db.insertSymbol("GOOGL", 1234, 75);
    if(sResult.errMsg == ""){
        cout << "Successfully inserted symbol with ID: " << sResult.accountID << " and symbol: " << sResult.symbol << endl;
    } else {
        std::cerr << sResult.errMsg << endl;
    }
}

void test_insertOpened(dbController& db){
    transOrderResult result;
    result = db.insertOpened(124,"GOOGL",1000,100);
    if(result.errMsg == ""){
        cout << "transid: " << result.transID << endl;
    } else {
        cerr << result.errMsg << endl;
    }
    result = db.insertOpened(123,"GOOGL",1000,100);
    if(result.errMsg == ""){
        cout << "transid: " << result.transID << endl;
    } else {
        cerr << result.errMsg << endl;
    }
    result = db.insertOpened(123,"GOOGL",50,10);
    if(result.errMsg == ""){
        cout << "transid: " << result.transID << endl;
    } else {
        cerr << result.errMsg << endl;
    }
    result = db.insertOpened(1234,"GOOGL",-100,100);
    if(result.errMsg == ""){
        cout << "transid: " << result.transID << endl;
    } else {
        cerr << result.errMsg << endl;
    }
    result = db.insertOpened(1234,"GOOGL",-75,100);
    if(result.errMsg == ""){
        cout << "transid: " << result.transID << endl;
    } else {
        cerr << result.errMsg << endl;
    }
}

void test_insertCanceled(dbController& db){
    transCancelResult result;
    result = db.insertCanceled(1);
    if(result.errMsg == ""){
        cout << "successfully canceled: " << result.canceledShares << " at: " << result.cancelTime << endl;
    } else {
        cerr << result.errMsg;
    }
    result = db.insertCanceled(3);
    if(result.errMsg == ""){
        cout << "successfully canceled: " << result.canceledShares << " at: " << result.cancelTime << endl;
    } else {
        cerr << result.errMsg;
    }
}

void test_queryShares(dbController& db){
    transQueryResult result;
    result = db.queryShares(1);
    if(result.errMsg == ""){
        cout << "opened: " << result.openedShares << " canceled: " << result.canceledShares << " at: " << result.cancelTime << endl;
    } else {
        cerr << result.errMsg;
    }
    result = db.queryShares(2);
    if(result.errMsg == ""){
        cout << "opened: " << result.openedShares << " canceled: " << result.canceledShares << " at: " << result.cancelTime << endl;
    } else {
        cerr << result.errMsg;
    }
    result = db.queryShares(3);
    if(result.errMsg == ""){
        cout << "opened: " << result.openedShares << " canceled: " << result.canceledShares << " at: " << result.cancelTime << endl;
    } else {
        cerr << result.errMsg;
    }
}


/*int main(){
    // Create an instance of dbController
    dbController db("exchange", "postgres", "passw0rd", "localhost", "5432");
    // Initialize the database (create tables if necessary)
    db.initializeAccount();
    db.initializeCanceled();
    db.initializeExecuted();
    db.initializeOpened();
    db.initializePosition();
    
    test_insertAccount(db);
    test_insertSymbol(db);
    test_insertOpened(db);
    test_insertCanceled(db);
    test_queryShares(db);
    return 0;
}*/
