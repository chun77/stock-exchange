#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "tinyxml2.h"

using namespace tinyxml2;
using namespace std;
using PairVec = vector<pair<int, float>>;

class xmlParser{
private:

    bool isParsable;
    bool isCreate;
    bool isTrans;

    vector<pair<int, float>> accountInfo;
    map<string, PairVec> symbolInfo;

public:

    const vector<pair<int, float>>& getAccountInfo() const {
        return accountInfo;
    }

    const map<string, PairVec>& getSymbolInfo() const {
        return symbolInfo;
    }

    void parse(const char* xmlString){
        XMLDocument doc;
        if(doc.Parse(xmlString) != XML_SUCCESS){
            return;
        }

        XMLElement* root = doc.FirstChildElement();
        if(!root){
            return;
        }

        if(string(root->Name()) == "create"){
            parseCreate(root);
        }
        //else if(string(root->Name()) == "transactions"){
          //  parseTransactions(root);
        //}
    }

    void parseCreate(XMLElement* element){
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

};


int main() {
    const char* xmlString = R"(
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

    xmlParser parser;
    parser.parse(xmlString);

    const vector<pair<int, float>>& accountInfo = parser.getAccountInfo();
    const map<string, PairVec>& symbolInfo = parser.getSymbolInfo();

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

    return 0;
}
