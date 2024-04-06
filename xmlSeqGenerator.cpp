#include "xmlSeqGenerator.hpp"

xmlSeqGenerator::xmlSeqGenerator(const string& rootName): doc(){
    XMLElement* root = doc.NewElement(rootName.c_str());
    doc.InsertEndChild(root);
}

string xmlSeqGenerator::getXML() const {
    XMLPrinter printer;
    doc.Print(&printer);
    string xmlString = printer.CStr();
    int xmlLength = xmlString.length();
    xmlString.insert(0, to_string(xmlLength) + "\n");
    return xmlString;
}

int xmlSeqGenerator::addElement(int accountID){
    XMLElement* account = doc.NewElement("created");
    account->SetAttribute("id", accountID);
    XMLElement* root = doc.RootElement();
    root->InsertEndChild(account);
    return 0;
}

int xmlSeqGenerator::addElement(int accountID, string errMsg){
    XMLElement* account = doc.NewElement("error");
    account->SetAttribute("id", accountID);
    account->SetText(errMsg.c_str());
    XMLElement* root = doc.RootElement();
    root->InsertEndChild(account);
    return 0;
}

int xmlSeqGenerator::addElement(string symbol, int accountID){
    XMLElement* symbolElement = doc.NewElement("created");
    symbolElement->SetAttribute("sym", symbol.c_str());
    symbolElement->SetAttribute("id", accountID);
    XMLElement* root = doc.RootElement();
    root->InsertEndChild(symbolElement);
    return 0;
}

int xmlSeqGenerator::addElement(string symbol, int accountID, string errMsg){
    XMLElement* symbolElement = doc.NewElement("error");
    symbolElement->SetAttribute("sym", symbol.c_str());
    symbolElement->SetAttribute("id", accountID);
    symbolElement->SetText(errMsg.c_str());
    XMLElement* root = doc.RootElement();
    root->InsertEndChild(symbolElement);
    return 0;
}

int xmlSeqGenerator::addElement(string symbol, float amount, float limit, int transID){
    XMLElement* orderElement = doc.NewElement("opened");
    orderElement->SetAttribute("sym", symbol.c_str());
    orderElement->SetAttribute("amount", amount);
    orderElement->SetAttribute("limit", limit);
    orderElement->SetAttribute("id", transID);
    XMLElement* root = doc.RootElement();
    root->InsertEndChild(orderElement);
    return 0;
}

int xmlSeqGenerator::addElement(string symbol, float amount, float limit, string errMsg){
    XMLElement* orderElement = doc.NewElement("error");
    orderElement->SetAttribute("sym", symbol.c_str());
    orderElement->SetAttribute("amount", amount);
    orderElement->SetAttribute("limit", limit);
    orderElement->SetText(errMsg.c_str());
    XMLElement* root = doc.RootElement();
    root->InsertEndChild(orderElement);
    return 0;
}

int xmlSeqGenerator::addElement(int transID, float oShares, float cShares, time_t cTime, executedShares xShares){
    XMLElement* queryElement = doc.NewElement("status");
    queryElement->SetAttribute("id", transID);

    if(oShares != 0){
        XMLElement* oElement = doc.NewElement("open");
        oElement->SetAttribute("shares", oShares);
        queryElement->InsertEndChild(oElement);
    }
    
    if(cShares != 0){
        XMLElement* canceledElement = doc.NewElement("canceled");
        canceledElement->SetAttribute("shares", cShares);
        canceledElement->SetAttribute("time", cTime);
        queryElement->InsertEndChild(canceledElement);
    }

    if(!xShares.empty()){
        for (const auto& executed : xShares) {
            XMLElement* executedElement = doc.NewElement("executed");
            executedElement->SetAttribute("shares", std::get<0>(executed));
            executedElement->SetAttribute("price", std::get<1>(executed));
            executedElement->SetAttribute("time", std::get<2>(executed));
            queryElement->InsertEndChild(executedElement);
        }
    }
    XMLElement* root = doc.RootElement();
    root->InsertEndChild(queryElement);
    return 0;
}

int xmlSeqGenerator::addElement(int transID, float cShares, time_t cTime, executedShares xShares){
    XMLElement* cancelElement = doc.NewElement("canceled");
    cancelElement->SetAttribute("id", transID);

    if(cShares != 0){
        XMLElement* canceledElement = doc.NewElement("canceled");
        canceledElement->SetAttribute("shares", cShares);
        canceledElement->SetAttribute("time", cTime);
        cancelElement->InsertEndChild(canceledElement);
    }
    
    if(!xShares.empty()){
        for (const auto& executed : xShares) {
            XMLElement* executedElement = doc.NewElement("executed");
            executedElement->SetAttribute("shares", std::get<0>(executed));
            executedElement->SetAttribute("price", std::get<1>(executed));
            executedElement->SetAttribute("time", std::get<2>(executed));
            cancelElement->InsertEndChild(executedElement);
        }
    }
    
    XMLElement* root = doc.RootElement();
    root->InsertEndChild(cancelElement);
    return 0;
}

int xmlSeqGenerator::addElement(int transID, float cShares, time_t cTime, executedShares xShares, string errMsg){
    XMLElement* errorElement = doc.NewElement("error");
    errorElement->SetAttribute("id", transID);
    errorElement->SetText(errMsg.c_str());
    XMLElement* root = doc.RootElement();
    root->InsertEndChild(errorElement);
    return 0;
}

/*int main(){
    xmlSeqGenerator generator("results");
    generator.addElement(1234);
    generator.addElement(1235);
    generator.addElement("oil", 9999, "no account");
    generator.addElement(1234, "already exist");
    generator.addElement("cake", 1235);

    string ans = generator.getXML();
    cout << ans;

    xmlSeqGenerator generator2("results");
    generator2.addElement("sym", 1.5, 2.8, 6);
    generator2.addElement("sym", 1.9, 2.6, "no account");
    generator2.addElement(1234, 100, {}, {});

    canceledShares cShares = {{50, 1609459200}, {30, 1609459300}};
    executedShares xShares = {{20, 150, 1609459400}, {25, 155, 1609459500}};
    generator2.addElement(1235, 200, cShares, xShares);

    canceledShares cSharesOnly = {{70, 1609459600}, {40, 1609459700}};
    generator2.addElement(1236, cSharesOnly, {});


    string ans2 = generator2.getXML();
    cout << ans2;
}*/