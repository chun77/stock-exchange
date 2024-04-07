#include "dbController.hpp"

createAccountResult dbController::insertAccount(int accountID, float balance){
    createAccountResult result;
    result.accountID = accountID;
    result.errMsg = "";
    while(true){
        try {
            transaction<serializable> txn(con);
            pqxx::result queryResult = txn.exec("SELECT accountID FROM Account WHERE accountID = " + std::to_string(accountID));
            if (!queryResult.empty()) {
                result.errMsg = "Account with ID " + std::to_string(accountID) + " already exists.";
                return result;
            }
            txn.exec("INSERT INTO Account (accountID, balance) VALUES (" + std::to_string(accountID) + ", " + std::to_string(balance) + ")");
            txn.commit();
            break;
        } catch (const exception& e) {
            continue;
        }
    }
    return result;
}

createSymResult dbController::insertSymbol(string symbol, int accountID, float NUM){
    createSymResult result;
    result.accountID = accountID;
    result.symbol = symbol;
    result.errMsg = "";
    while(true){
        try{
            transaction<serializable> txn(con);
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
            break;
        } catch (const exception& e) {
            continue;
        }
    }
    
    return result;
}

transOrderResult dbController::insertOpened(int accountID, string symbol, float amt, float limit){
    transOrderResult result;
    result.symbol = symbol;
    result.limit = limit;
    result.amount = amt;
    result.errMsg = "";
    while(true){
        try {
            transaction<serializable> txn(con);
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

            // match orders
            matchOrders(txn, accountID, transIDRes[0][0].as<int>(), symbol, amt, limit);
            // Commit transaction
            txn.commit();
            // Set result transID
            result.transID = transIDRes[0][0].as<int>();
            break;
        } catch (const exception& e) {
            continue;
        }
    }
    
    return result;
}

void dbController::matchOrders(transaction<serializable>& txn, int accountID, int newTransID, const string& symbol, float amt, float limit) {
    float remainingAmt = amt;
    while(remainingAmt != 0){ //只要还有剩余的amt没match就要尝试match
        try {
            string priceMatchCondition;
            string sql;
            if (amt > 0) {
                priceMatchCondition = "price_limit <= " + txn.quote(limit);
                sql = "SELECT transID, accountID, symbol, amt, price_limit FROM OpenedOrder "
                "WHERE symbol = " + txn.quote(symbol) + " "
                "AND amt * " + txn.quote(amt) + " < 0 " 
                "AND " + priceMatchCondition + " " 
                "AND accountID != " + to_string(accountID) + " " 
                "ORDER BY price_limit ASC, time ASC "
                "LIMIT 1";
                // withoud LIMIT 1? Because we may need to match many orders?
            } else {
                priceMatchCondition = "price_limit >= " + txn.quote(limit);
                sql = "SELECT transID, accountID, symbol, amt, price_limit FROM OpenedOrder "
                "WHERE symbol = " + txn.quote(symbol) + " "
                "AND amt * " + txn.quote(amt) + " < 0 " 
                "AND " + priceMatchCondition + " " 
                "AND accountID != " + to_string(accountID) + " "
                "ORDER BY price_limit DESC, time ASC "
                "LIMIT 1";
            }

            pqxx::result res = txn.exec(sql);
            if (!res.empty()) {
                auto row = res[0];
                int matchTransID = row["transID"].as<int>();
                int matchAccountID = row["accountID"].as<int>();
                float matchAmt = std::abs(row["amt"].as<float>());
                float matchLimit = row["price_limit"].as<float>();

                float tradeAmt = std::min(std::abs(amt), matchAmt);
                float executionPrice = matchLimit; 

                if (amt > 0) {
                    updateBuyerPosition(txn, accountID, symbol, tradeAmt);
                    updateBuyerAccount(txn, accountID, (limit - executionPrice) * tradeAmt);
                    updateSellerAccount(txn, matchAccountID, executionPrice * tradeAmt);
                    updateOpened(txn, newTransID, tradeAmt);
                    updateOpened(txn, matchTransID, -tradeAmt);
                    updateExecuted(txn, newTransID, accountID, symbol, executionPrice, tradeAmt);
                    updateExecuted(txn, matchTransID, matchAccountID, symbol, executionPrice, -tradeAmt);
                } else {
                    updateSellerAccount(txn, accountID, executionPrice * tradeAmt);
                    updateBuyerPosition(txn, matchAccountID, symbol, tradeAmt);
                    updateOpened(txn, newTransID, -tradeAmt);
                    updateOpened(txn, matchTransID, tradeAmt);
                    updateExecuted(txn, newTransID, accountID, symbol, executionPrice, -tradeAmt);
                    updateExecuted(txn, matchTransID, matchAccountID, symbol, executionPrice, tradeAmt);
                }
                if(remainingAmt > 0){
                    remainingAmt = remainingAmt - tradeAmt;
                } else {
                    remainingAmt = remainingAmt + tradeAmt;
                }
                
            } else {return;}
            // 如果没有order可以match了就直接退出
        } catch (const exception& e) {
            throw;
        }
    }
}

void dbController::updateBuyerPosition(transaction<serializable>& txn, int accountID, const string& symbol, float amt) {
    string checkExistenceSql = "SELECT COUNT(*) AS count FROM Position WHERE accountID = " + txn.quote(accountID) + " AND symbol = " + txn.quote(symbol);
    pqxx::result existenceResult = txn.exec(checkExistenceSql);
    int rowCount = existenceResult[0]["count"].as<int>();
    if (rowCount > 0) {
        txn.exec("UPDATE Position SET NUM = NUM + " + txn.quote(amt) + " WHERE accountID = " + txn.quote(accountID) + " AND symbol = " + txn.quote(symbol));
    } else {
        txn.exec("INSERT INTO Position (symbol, accountID, NUM) VALUES (" + txn.quote(symbol)+ ", " + txn.quote(accountID) + ", " + txn.quote(amt) + ")");
    }
}

void dbController::updateBuyerAccount(transaction<serializable>& txn, int accountID, float amount) {
    txn.exec("UPDATE Account SET balance = balance + " + txn.quote(amount) + " WHERE accountID = " + txn.quote(accountID));
}

void dbController::updateSellerAccount(transaction<serializable>& txn, int accountID, float amount) {
    txn.exec("UPDATE Account SET balance = balance + " + txn.quote(amount) + " WHERE accountID = " + txn.quote(accountID));
}

void dbController::updateOpened(transaction<serializable>& txn, int transID, float amt){
    string selectSql = "SELECT * FROM OpenedOrder WHERE transID = " + txn.quote(transID);
    result result = txn.exec(selectSql);
    if (!result.empty()) {
        auto row = result[0];
        float currentAmt = row["amt"].as<float>();
        float updatedAmt = currentAmt - amt;
        if (updatedAmt <= 0) {
            string deleteSql = "DELETE FROM OpenedOrder WHERE transID = " + txn.quote(transID);
            txn.exec(deleteSql);
        } else {
            string updateSql = "UPDATE OpenedOrder SET amt = " + txn.quote(updatedAmt) + " WHERE transID = " + txn.quote(transID);
            txn.exec(updateSql);
        }
    } else {
        std::cout << "No record found with transID: " << transID << std::endl;
    }
}

void dbController::updateExecuted(transaction<serializable>& txn, int transID, int accountID, const string& symbol, float price, float amt){
    string insertSql = "INSERT INTO ExecutedOrder (transID, accountID, symbol, price, amt, time) VALUES ("
                       + txn.quote(transID) + ", "
                       + txn.quote(accountID) + ", "
                       + txn.quote(symbol) + ", "
                       + txn.quote(price) + ", "
                       + txn.quote(amt) + ", " 
                       + txn.quote(time(NULL)) + ")";
    txn.exec(insertSql);
}

transCancelResult dbController::insertCanceled(int accountID, int transID){
    transCancelResult result;
    result.transID = transID;
    result.canceledShares = 0;
    result.errMsg = "";
    result.executedShares = {};
    while(true){
        try {
            transaction<serializable> txn(con);
            pqxx::result accountResult = txn.exec("SELECT accountID FROM Account WHERE accountID = " + std::to_string(accountID));
            if (accountResult.empty()) {
                result.errMsg = "This accountID doesn't exist";
                return result;
            }
            pqxx::result res = txn.exec("SELECT * FROM OpenedOrder WHERE transID = " + txn.quote(transID) + " AND accountID = " + txn.quote(accountID));
            if (res.empty()) {
                result.errMsg = "No transaction found with transID: " + std::to_string(transID);
                return result;
            }
            // Get details of the transaction
            float canceledShares;
            time_t cancelTime = time(NULL);
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
            if(!executedRes.empty()){
                for (const auto& row : executedRes) {
                    float amt = row["amt"].as<float>();
                    float price = row["price"].as<float>();
                    time_t t = row["time"].as<time_t>();
                    result.executedShares.push_back(std::make_tuple(amt, price, t));
                }
            }
            txn.commit();
            break;
        } catch (const std::exception& e) {
            continue;
        }
    }
    return result;
}

transQueryResult dbController::queryShares(int accountID, int transID){
    transQueryResult result;
    result.transID = transID;
    result.openedShares = 0;
    result.canceledShares = 0;
    result.executedShares = {};
    result.errMsg = "";
    while(true){
        try {
            transaction<serializable> txn(con);
            pqxx::result accountResult = txn.exec("SELECT accountID FROM Account WHERE accountID = " + std::to_string(accountID));
            if (accountResult.empty()) {
                result.errMsg = "This accountID doesn't exist";
                return result;
            }
            pqxx::result openedRes = txn.exec(
                "SELECT amt FROM OpenedOrder WHERE transID = " + txn.quote(transID) + " AND accountID = " + txn.quote(accountID)
            );
            pqxx::result canceledRes = txn.exec(
                "SELECT amt, time FROM CanceledOrder WHERE transID = " + txn.quote(transID) + " AND accountID = " + txn.quote(accountID)
            );
            pqxx::result executedRes = txn.exec(
                "SELECT amt, price, time FROM ExecutedOrder WHERE transID = " + txn.quote(transID) + " AND accountID = " + txn.quote(accountID)
            );
            if(openedRes.empty() && canceledRes.empty() && executedRes.empty()){
                result.errMsg = "Invalid trans_ID";
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
            break;
        } catch (const std::exception& e) {
            std::cerr << "Error querying shares: " << e.what() << std::endl;
            continue;
        }
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
            "   transID INT,"
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

/*
void test_insertAccount(dbController& db){
    createAccountResult result = db.insertAccount(123, 1000000);
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
    result = db.insertAccount(1234, 1000000);
    // Handle the result
    if (result.errMsg == "") {
        std::cout << "Successfully inserted account with ID: " << result.accountID << std::endl;
    } else {
        std::cerr << "Failed to insert account with ID: " << result.accountID << ". Error: " << result.errMsg << std::endl;
    }
    result = db.insertAccount(12345, 1000000);
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
    sResult = db.insertSymbol("GOOGL", 1234, 125);
    if(sResult.errMsg == ""){
        cout << "Successfully inserted symbol with ID: " << sResult.accountID << " and symbol: " << sResult.symbol << endl;
    } else {
        std::cerr << sResult.errMsg << endl;
    }
    sResult = db.insertSymbol("GOOGL", 123, 150);
    if(sResult.errMsg == ""){
        cout << "Successfully inserted symbol with ID: " << sResult.accountID << " and symbol: " << sResult.symbol << endl;
    } else {
        std::cerr << sResult.errMsg << endl;
    }
    sResult = db.insertSymbol("GOOGL", 12345, 300);
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
    result = db.insertOpened(1234,"GOOGL",-100,80);
    if(result.errMsg == ""){
        cout << "transid: " << result.transID << endl;
    } else {
        cerr << result.errMsg << endl;
    }
    result = db.insertOpened(1234,"GOOGL",-75,10);
    if(result.errMsg == ""){
        cout << "transid: " << result.transID << endl;
    } else {
        cerr << result.errMsg << endl;
    }
    result = db.insertOpened(12345,"GOOGL",-300,90);
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
        vector<tuple<float, float, time_t>> executedShares = result.executedShares;
        cout << "Executed: " << endl;
        for (const auto& share : executedShares) {
            float shares = get<0>(share);
            float price = get<1>(share);
            time_t time = get<2>(share);
            cout << "Shares: " << shares << ", Price: " << price << ", Time: " << ctime(&time) << endl;
        }
    } else {
        cerr << result.errMsg;
    }
    result = db.queryShares(2);
    if(result.errMsg == ""){
        cout << "opened: " << result.openedShares << " canceled: " << result.canceledShares << " at: " << result.cancelTime << endl;
        vector<tuple<float, float, time_t>> executedShares = result.executedShares;
        cout << "Executed: " << endl;
        for (const auto& share : executedShares) {
            float shares = get<0>(share);
            float price = get<1>(share);
            time_t time = get<2>(share);
            cout << "Shares: " << shares << ", Price: " << price << ", Time: " << ctime(&time) << endl;
        }
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

int main(){
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
