//
//  main.cpp
//  jssy
//
//  Created by tihmstar on 15.09.17.
//  Copyright © 2017 tihmstar. All rights reserved.
//

#include <iostream>
#include <stdlib.h>
#include "jssy.hpp"

using namespace std;

int cppmain(int argc, const char * argv[]) {

    jssycpp::jssy jc("firmwares.json"); // place https://api.ipsw.me/v2.1/firmwares.json here
    
    for (auto dev : jc["devices"]){
        cout << dev.stringValue() << endl;
    }
    
    
    return 0;
}
