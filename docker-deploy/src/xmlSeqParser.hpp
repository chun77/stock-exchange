#ifndef XMLSEQPARSER_HPP
#define XMLSEQPARSER_HPP

#include <iostream>
#include <tuple>
#include <vector>
#include <map>
#include "tinyxml2.h"

using namespace tinyxml2;
using namespace std;

using PairVec = vector<pair<int, float>>;

class xmlSeqParser{
private:
    XMLDocument doc;
    pair<int, float> accountInfo;
    map<string, PairVec> symbolInfo;

    int accoutIdForTrans;
    tuple<string, float, float> orderInfo;
    int queryID;
    int cancelID;

    XMLElement* currElement;

public:
    int parse(const char * xmlString);
    int getNextCreate();
    int getNextTrans();

    const pair<int, float>& getAccountInfo() const;
    const map<string, PairVec>& getSymbolInfo() const;
    const int& getAccountIdForTrans() const;
    const tuple<string, float, float>& getOrderInfo() const;
    const int& getQueryID() const;
    const int& getCancelID() const;
};

#endif 