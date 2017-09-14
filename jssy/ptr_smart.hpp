//
//  ptr_smart.hpp
//  cookieServer
//
//  Created by tihmstar on 21.07.17.
//  Copyright © 2017 tihmstar. All rights reserved.
//

#ifndef ptr_smart_hpp
#define ptr_smart_hpp

#include <functional>

extern "C"{
#include <stdlib.h>
}

template <typename T>
class ptr_smart {
    std::function<void(T)> _ptr_free = NULL;
    T p;
public:
    ptr_smart(T pp = NULL, std::function<void(T)> ptr_free = NULL){
        static_assert(std::is_pointer<T>(), "error: this is for pointers only\n");
        _ptr_free = ptr_free;
    }
    T operator=(T pp) {return p = pp;}
    T *operator&() {return &p;}
    T operator->() {return p;}
    operator const T() const {return p;}
    ~ptr_smart() {
        if (p) {
            if (_ptr_free)
                _ptr_free(p);
            else
                free((void*)p);
        };
    }
};

#endif /* ptr_smart_hpp */
