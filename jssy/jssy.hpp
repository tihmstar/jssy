//
//  jssy.hpp
//  jssy
//
//  Created by tihmstar on 21.07.17.
//  Copyright © 2017 tihmstar. All rights reserved.
//

#ifndef jssy_hpp
#define jssy_hpp

#include <string>
#include <exception>
#include "ptr_smart.hpp"

namespace jssycpp {
    namespace c{
        extern "C"{
            #include "jssy.h"
        }
    }
    class jssy{
        
        
        const c::jssytok_t * _token;
        long _cnt;
    public:
        class iterator{
            jssy *_tok;
            size_t _s;
            bool _isDict;
        public:
            iterator(const jssy& tok, size_t s, bool isDict):_s(s),_isDict(isDict){
                _tok = new jssy(tok);
                if (_isDict) _s/=2;
            };
            bool operator!=(const iterator& token){
                return _s--;
            }
            iterator& operator++(){
                _tok->_token = _tok->_token->next;
                return *this;
            }
            jssy& operator*(){return *_tok;}
            
            ~iterator(){delete _tok;}
        };
        jssy(std::string filename);
        jssy(const c::jssytok_t *token);
        jssy(const jssy& token);
        
        c::jssytype_t type() const;
        size_t size() const;
        std::string stringValue() const;
        double numval() const;
        jssy subval() const;
        jssy operator [](int index) const;
        jssy operator [](std::string key) const;
        
        explicit operator bool() const;
        
        iterator begin() const;
        iterator end() const;
        
        
        
        explicit operator std::string() const;
        explicit operator double() const;
        
        ~jssy();
        
    };
}


#endif /* jssy_hpp */
