//
//  helper.h
//  jssy
//
//  Created by tihmstar on 21.07.17.
//  Copyright © 2017 tihmstar. All rights reserved.
//

#ifndef helper_h
#define helper_h


#define reterror(err) throw jssyException(__LINE__,err)
#define assure(cond) if ((cond) == 0) throw jssyException(__LINE__, "assure failed")
#define retassure(cond, err) if ((cond) == 0) throw jssyException(__LINE__,err)

class jssyException : public std::exception{
    std::string _err;
    int _code;
public:
    jssyException(int code, const std::string &err) : _err(err), _code(code) {};
    jssyException(const std::string &err) : _err(err), _code(0) {};
    jssyException(int code) : _code(code) {};
    const char *what(){return _err.c_str();}
    int code(){return _code;}
};

#endif /* helper_h */
