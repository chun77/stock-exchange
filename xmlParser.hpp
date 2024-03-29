#ifndef XML_PARSER_H
#define XML_PARSER_H

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "tinyxml2.h"

using namespace tinyxml2;
using namespace std;

using PairVec = vector<pair<int, float>>;

class xmlParser {
private:
    bool isParsable;
    bool isCreate;
    bool isTrans;

    vector<pair<int, float>> accountInfo;
    map<string, PairVec> symbolInfo;

    int accoutIdForTrans;
    vector<tuple<string, float, float>> orderInfo;
    vector<int> queryIDs;
    vector<int> cancelIDs;

public:
    const vector<pair<int, float>>& getAccountInfo() const;
    const map<string, PairVec>& getSymbolInfo() const;
    const int& getAccountIdForTrans() const;
    const vector<tuple<string, float, float>>& getOrderInfo() const;
    const vector<int> getQueryIDs() const;
    const vector<int> getCancelIDs() const;
    const bool& getIsCreate() const;
    const bool& getIsTrans() const;

    void parse(const char* xmlString);
    void parseCreate(XMLElement* element);
    void parseTransactions(XMLElement* element);
};

#endif
