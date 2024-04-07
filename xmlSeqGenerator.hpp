#ifndef XMLSEQGENERATOR_HPP
#define XMLSEQGENERATOR_HPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "tinyxml2.h"

using namespace tinyxml2;
using namespace std;
using canceledShares = vector<pair<float, time_t>>;
using executedShares = vector<tuple<float, float, time_t>>;

class xmlSeqGenerator{
private:
    XMLDocument doc;

public:
    xmlSeqGenerator(const string& rootName);
    ~xmlSeqGenerator(){}
    string getXML() const;
    // create Response
    int addElement(int accountID);
    int addElement(int accountID, string errMsg);
    int addElement(string symbol, int accountID);
    int addElement(string symbol, int accountID, string errMsg);
    // transactions Response
    int addElement(string symbol, float amount, float limit, int transID);
    int addElement(string symbol, float amount, float limit, string errMsg);
    int addElement(int transID, float oShares, float cShares, time_t cTime, executedShares xShares);
    int addElement(int transID, float oShares, float cShares, time_t cTime, executedShares xShares, string errMsg);
    int addElement(int transID, float cShares, time_t cTime, executedShares xShares);
    int addElement(int transID, float cShares, time_t cTime, executedShares xShares, string errMsg);
};

#endif 