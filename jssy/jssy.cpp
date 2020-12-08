//
//  jssy.cpp
//  jssy
//
//  Created by tihmstar on 21.07.17.
//  Copyright © 2017 tihmstar. All rights reserved.
//

#include "jssy.hpp"
#include <fstream>
#include "helper.h"

extern "C"{
#include <string.h>
}

using namespace std;
using namespace jssycpp;

std::string ReplaceAll(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

jssy::jssy(string filename){
    fstream file;
    file.open(filename);
        
    std::string buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    assure((_cnt = c::jssy_parse(buffer.c_str(), buffer.size(), NULL, NULL)) > 0);
    
    assure(_token = (c::jssytok_t*)malloc(_cnt * sizeof(c::jssytok_t)));
    
    assure((_cnt = c::jssy_parse(buffer.c_str(), buffer.size(), (c::jssytok_t*)_token, _cnt * sizeof(c::jssytok_t))) > 0);
}


jssy::jssy(const c::jssytok_t *token) :_token(token), _cnt(0) {}
jssy::jssy(const jssy& token) : _token(token._token),_cnt(0) {}


jssy::~jssy(){
    if (_cnt)
        free((c::jssytok_t*)_token);
}

c::jssytype_t jssy::type() const{
    return _token->type;
}

size_t jssy::size() const{
    return _token->size;
}

std::string jssy::stringValue() const{
    retassure(_token->type == c::JSSY_STRING || _token->type == c::JSSY_DICT_KEY, "not a string");
    string rt(_token->value,_token->size);
    rt = ReplaceAll(rt, "\\\"", "\"");
    rt = ReplaceAll(rt, "\\\\", "\\");
    rt = ReplaceAll(rt, "\\\t", "\t");
    rt = ReplaceAll(rt, "\\\r", "\r");
    rt = ReplaceAll(rt, "\\\n", "\n");
    return rt;
}

jssy jssy::subval() const{
    assure(_token->type == c::JSSY_DICT_KEY);
    return {_token->subval};
}

double jssy::numval() const{
    retassure(_token->type == c::JSSY_PRIMITIVE, "not a number");
    return _token->numval;
}

jssy::operator double() const{
    return numval();
}

jssy::operator string() const{
    return stringValue();
}

jssy::operator bool() const{
    return _token;
}

jssy::iterator jssy::begin() const{
    return {(*this)[0],_token->size,_token->type == c::JSSY_DICT};
}

jssy::iterator jssy::end() const{
    return {NULL,0,0};
}

jssy jssy::operator [](int index) const{
    retassure(_token->type == c::JSSY_ARRAY || _token->type == c::JSSY_DICT, "not a list");
    size_t s = _token->size;
    const c::jssytok_t *cur = _token->subval;
    while (index--) {
        assure(s--);
        cur=cur->next;
    }
    return {cur};
}

jssy jssy::operator [](string key) const{
    retassure(_token->type == c::JSSY_DICT, "not a dict");
    
    c::jssytok_t *tmp = _token->subval;
    for (size_t i = 0; i< _token->size; i++){
        if (tmp->size == key.size() && strncmp(tmp->value, key.c_str(), tmp->size) == 0)
            return {tmp->subval};
        tmp = tmp->next;
    }
    return {NULL};
}






