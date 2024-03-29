#ifndef XML_GENERATOR_H
#define XML_GENERATOR_H

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "tinyxml2.h"

using namespace tinyxml2;
using namespace std;

using responseItems = vector<tuple< int, string, string>>;

class xmlGenerator
{
public:
    string createResponse(responseItems items);
};

string xmlGenerator::createResponse(responseItems items)
{
    cout<<"enter generator"<<endl;
    XMLDocument doc;
    XMLElement* root = doc.NewElement("results");
    doc.InsertFirstChild(root);

    for (auto& item : items) {
        int accountID = get<0>(item);
        string symbol = get<1>(item);
        string err = get<2>(item);

        if (err.empty()) { // no error, create success
            XMLElement* createdElement = doc.NewElement("created");
            if (symbol.empty()) { // create account
                createdElement->SetAttribute("id", to_string(accountID).c_str());
            } else { // create symbol
                createdElement->SetAttribute("sym", symbol.c_str());
                createdElement->SetAttribute("id", to_string(accountID).c_str());
            }
            root->InsertEndChild(createdElement);
        } else { // error occur
            XMLElement* errorElement = doc.NewElement("error");
            if (symbol.empty()) { //err creating account
                errorElement->SetAttribute("id", to_string(accountID).c_str());
            } else { // err creating sym
                errorElement->SetAttribute("sym", symbol.c_str());
                errorElement->SetAttribute("id", to_string(accountID).c_str());
            }
            errorElement->SetText(err.c_str());
            root->InsertEndChild(errorElement);
        }
    }

    XMLPrinter printer;
    doc.Accept(&printer);
    string s = string(printer.CStr());
    return s;
    
}

#endif