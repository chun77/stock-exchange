#include "xmlSeqParser.hpp"

int xmlSeqParser::parse(const char* xmlString){
    if(doc.Parse(xmlString) != XML_SUCCESS){
        return -1;
    }

    XMLElement* root = doc.FirstChildElement();
    if(!root){
        return -1;
    }

    if(string(root->Name()) == "create"){
        currElement = root;
        return 0;
    }
    else if(string(root->Name()) == "transactions"){
        currElement = root;
        accoutIdForTrans = stoi(root->Attribute("id"));
        return 1;
    }
    else{
        return -1;
    }
}

int xmlSeqParser::getNextCreate(){
    XMLElement* child;
    if(strcmp(currElement->Name(), "create") == 0){
        child = currElement->FirstChildElement();
    }
    else{
        child = currElement->NextSiblingElement();
    }
    currElement = child;

    if(child == NULL){
        return -1;
    }
    else if(string(child->Name()) == "account"){
        int accountID = stoi(child->Attribute("id"));
        float balance = stof(child->Attribute("balance"));
        accountInfo = make_pair(accountID, balance);
        return 0;
    }
    else if(string(child->Name()) == "symbol"){
        symbolInfo.clear();
        string symbol = child->Attribute("sym");
        for(XMLElement* symbolChild = child->FirstChildElement(); symbolChild; symbolChild = symbolChild->NextSiblingElement()){
            int accountID = stoi(symbolChild->Attribute("id"));
            float NUM = stof(symbolChild->GetText());
            symbolInfo[symbol].push_back(make_pair(accountID, NUM));
        }
        return 1;
    }
    else{
        return -1;
    }
}

int xmlSeqParser::getNextTrans(){
    XMLElement* child;
    if(strcmp(currElement->Name(), "transactions") == 0){
        accoutIdForTrans = stoi(currElement->Attribute("id"));
        child = currElement->FirstChildElement();
    }
    else{
        child = currElement->NextSiblingElement();
    }
    currElement = child;

    if(child == NULL){
        return -1;
    }
    else if(string(child->Name()) == "order"){
        string symbol = child->Attribute("sym");
        float amount = stof(child->Attribute("amount"));
        float limit = stof(child->Attribute("limit"));
        orderInfo = make_tuple(symbol, amount, limit);
        return 0;
    }
    else if(string(child->Name()) == "query"){
        int id = stoi(child->Attribute("id"));
        queryID = id;
        return 1;
    }
    else if(string(child->Name()) == "cancel"){
        int id = stoi(child->Attribute("id"));
        cancelID = id;
        return 2;
    }
    else{
        return -1;
    }
}

const pair<int, float>& xmlSeqParser::getAccountInfo() const {
    return accountInfo;
}

const map<string, PairVec>& xmlSeqParser::getSymbolInfo() const {
    return symbolInfo;
}

const int& xmlSeqParser::getAccountIdForTrans() const {
    return accoutIdForTrans;
}

const tuple<string, float, float>& xmlSeqParser::getOrderInfo() const {
    return orderInfo;
}

const int& xmlSeqParser::getQueryID() const {
    return queryID;
}

const int& xmlSeqParser::getCancelID() const {
    return cancelID;
}


/*int main() {
    const char* createString = R"(
        <create>
            <account id="1" balance="100.5"/>
            <symbol sym="SYM2">
                <account id="1">70.5</account>
                <account id="2">80.5</account>
            </symbol>
            <account id="2" balance="200.5"/>
            <symbol sym="SYM1">
                <account id="1">50.5</account>
                <account id="2">60.5</account>
            </symbol>
        </create>
    )";

    const char* transactionXmlString = R"(
        <transactions id="123">
            <order sym="SYM1" amount="100.0" limit="10.0"/>
            <query id="456"/>
            <cancel id="789"/>
            <order sym="SYM2" amount="200.0" limit="20.0"/>
            <query id="456"/>
            <cancel id="789"/>
        </transactions>
    )";

    XMLDocument doc;
    xmlSeqParser parser;
    int result = parser.parse(createString);
    if(result == 0){
        while(true){
            int next = parser.getNextCreate();
            if(next == 0){
                pair<int, float> pair = parser.getAccountInfo();
                cout << "Account ID: " << pair.first << ", Balance: " << pair.second << endl;
            }
            else if(next == 1){
                map<string, PairVec> map = parser.getSymbolInfo();
                const auto& pair = *map.begin();
                cout << "Symbol: " << pair.first << endl;
                const PairVec& vec = pair.second;
                for (const auto& subPair : vec) {
                    cout << "Account ID: " << subPair.first << ", NUM: " << subPair.second << endl;
                }
            }
            else if(next == -1){
                break;
            }
        }
    }

    result = parser.parse(transactionXmlString);
    if(result == 1){
        cout << "account id: " << parser.getAccountIdForTrans() << endl;
        while(true){
            int next = parser.getNextTrans();
            if(next == -1){
                break;
            }
            else if(next == 0){
                tuple<string, float, float> order = parser.getOrderInfo();
                cout << "Symbol: " << get<0>(order) << ", ";
                cout << "Amount: " << get<1>(order) << ", ";
                cout << "Limit: " << get<2>(order) << endl;
            }
            else if(next == 1){
                int queryID = parser.getQueryID();
                cout << "Query ID: " << queryID << endl;
            }
            else if(next == 2){
                int cancelID = parser.getCancelID();
                cout << "Cancel ID: " << cancelID << endl;
            }
        }
    }

    return 0;
}*/