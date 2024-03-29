#include "xmlGenerator.hpp"
#include<iostream>

int main(){
    xmlGenerator generator;
    responseItems testItems;
    testItems.push_back(make_tuple(123456,"",""));
    testItems.push_back(make_tuple(123456,"SYM",""));
    testItems.push_back(make_tuple(123456,"","Account exists"));
    testItems.push_back(make_tuple(777777,"SYM","Account dosen't exist"));

    string res = generator.createResponse(testItems);
    cout<<res;
    return 1;
}