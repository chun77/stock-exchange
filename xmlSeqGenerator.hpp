#ifndef XMLSEQGENERATOR_HPP
#define XMLSEQGENERATOR_HPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "tinyxml2.h"

using namespace tinyxml2;
using namespace std;

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
    // TODO
    // 剩下query和cancel
};

#endif 