#include "xmlParser.hpp"

const vector<pair<int, float>>& xmlParser::getAccountInfo() const {
    return accountInfo;
}

const map<string, PairVec>& xmlParser::getSymbolInfo() const {
    return symbolInfo;
}

const int& xmlParser::getAccountIdForTrans() const {
    return accoutIdForTrans;
}

const vector<tuple<string, float, float>>& xmlParser::getOrderInfo() const {
    return orderInfo;
}

const vector<int> xmlParser::getQueryIDs() const {
    return queryIDs;
}

const vector<int> xmlParser::getCancelIDs() const {
    return cancelIDs;
}

const bool& xmlParser::getIsCreate() const {
    return isCreate;
}

const bool& xmlParser::getIsTrans() const {
    return isTrans;
}

void xmlParser::parse(const char* xmlString){
    XMLDocument doc;
    if(doc.Parse(xmlString) != XML_SUCCESS){
        return;
    }

    XMLElement* root = doc.FirstChildElement();
    if(!root){
        return;
    }

    if(string(root->Name()) == "create"){
        isCreate = true;
        isTrans = false;
        parseCreate(root);
    }
    else if(string(root->Name()) == "transactions"){
        isCreate = false;
        isTrans = true;
        accoutIdForTrans = stoi(root->Attribute("id"));
        parseTransactions(root);
    }
}

void xmlParser::parseCreate(XMLElement* element){
    for(XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()){
        if(string(child->Name()) == "account"){
            int accountID = stoi(child->Attribute("id"));
            float balance = stof(child->Attribute("balance"));
            accountInfo.push_back(make_pair(accountID, balance));
        }
        else if(string(child->Name()) == "symbol"){
            string symbol = child->Attribute("sym");
            for(XMLElement* symbolChild = child->FirstChildElement(); symbolChild; symbolChild = symbolChild->NextSiblingElement()){
                int accountID = stoi(symbolChild->Attribute("id"));
                float NUM = stof(symbolChild->GetText());
                symbolInfo[symbol].push_back(make_pair(accountID, NUM));
            }
        }
    }
}

void xmlParser::parseTransactions(XMLElement* element){
    for(XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()){
        if(string(child->Name()) == "order"){
            string symbol = child->Attribute("sym");
            float amount = stof(child->Attribute("amount"));
            float limit = stof(child->Attribute("limit"));
            orderInfo.push_back(make_tuple(symbol, amount, limit));
        }
        else if(string(child->Name()) == "query"){
            int id = stoi(child->Attribute("id"));
            queryIDs.push_back(id);
        }
        else if(string(child->Name()) == "cancel"){
            int id = stoi(child->Attribute("id"));
            cancelIDs.push_back(id);
        }
    }
}


/*int main() {
    const char* createString = R"(
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

    const char* transactionXmlString = R"(
        <transactions id="123">
            <order sym="SYM1" amount="100.0" limit="10.0"/>
            <order sym="SYM2" amount="200.0" limit="20.0"/>
            <query id="456"/>
            <query id="456"/>
            <cancel id="789"/>
            <cancel id="789"/>
        </transactions>
    )";

    xmlParser parser;
    parser.parse(createString);
    parser.parse(transactionXmlString);

    const vector<pair<int, float>>& accountInfo = parser.getAccountInfo();
    const map<string, PairVec>& symbolInfo = parser.getSymbolInfo();

    // test result for create String
    cout << "Account Info:" << endl;
    for (const auto& pair : accountInfo) {
        cout << "Account ID: " << pair.first << ", Balance: " << pair.second << endl;
    }

    cout << endl << "Symbol Info:" << endl;
    for (const auto& pair : symbolInfo) {
        cout << "Symbol: " << pair.first << endl;
        const PairVec& vec = pair.second;
        for (const auto& subPair : vec) {
            cout << "Account ID: " << subPair.first << ", NUM: " << subPair.second << endl;
        }
    }

    // test result for transactions String
    cout << endl << "Transaction Info:" << endl;
    cout << "account id: " << parser.getAccountIdForTrans() << endl;
    cout << "Orders:" << endl;
    for (const auto& order : parser.getOrderInfo()) {
        cout << "Symbol: " << get<0>(order) << ", ";
        cout << "Amount: " << get<1>(order) << ", ";
        cout << "Limit: " << get<2>(order) << endl;
    }

    cout << endl << "Queries:" << endl;
    for (int queryID : parser.getQueryIDs()) {
        cout << "Query ID: " << queryID << endl;
    }

    cout << endl << "Cancellations:" << endl;
    for (int cancelID : parser.getCancelIDs()) {
        cout << "Cancel ID: " << cancelID << endl;
    }

    return 0;
}*/
