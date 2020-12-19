//
//  jssy_create.c
//  jssy
//
//  Created by tihmstar on 07.12.20.
//  Copyright © 2020 tihmstar. All rights reserved.
//

#include "jssy_create.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct jssy_create_tok_private_t{
    jssytype_t type;
    size_t size;
    union{
        char *value;
        double numval;
#ifdef HAVE_JSSY_BOOL
        unsigned char boolval;
#endif
    };
    struct jssy_create_tok_private_t *subval;
    struct jssy_create_tok_private_t *next;
    struct jssy_create_tok_private_t *prev;
} jssy_create_tok_private_t;

#define PRIMITIVE_PRINT_LIMIT 20

#define assure(cond) do {if (!(cond)){err = __LINE__; goto error; }} while(0)
#define safeFree(ptr) do {if (ptr){free((void*)ptr); ptr = NULL; }} while(0)
#define safeFreeCustom(ptr,func) do {if (ptr){func(ptr); ptr = NULL; }} while(0)

jssy_create_tok_t jssy_new_tok(jssytype_t type){
    int err = 0;
    jssy_create_tok_t ret = NULL;
    assure(ret = (jssy_create_tok_t)malloc(sizeof(struct jssy_create_tok_private_t)));
    memset(ret, 0, sizeof(struct jssy_create_tok_private_t));
    
    ret->type = type;
error:
    if (err) {
        safeFree(ret);
        return NULL;
    }
    return ret;
}

size_t jssy_dump_size(jssy_create_tok_t obj){
    int err = 0;
    size_t ret = 0;
    
    switch (obj->type) {
        case JSSY_PRIMITIVE:
            ret = PRIMITIVE_PRINT_LIMIT;
            break;
        case JSSY_STRING:
            ret = obj->size+2;//"*2
            break;
        case JSSY_ARRAY:
        case JSSY_DICT:
        {
            ret = 2; //[] / {}
            jssy_create_tok_t v = obj->subval;
            for (size_t i=0; i<obj->size; i++, v = v->next) {
                size_t vs = 0;
                assure(vs = jssy_dump_size(v)); //object cannot be of size 0
                ret += vs+1;// add , every time
            }
            break;
        }
        case JSSY_DICT_KEY:
        {
            ret = 1; // :
            ret += obj->size+2;
            size_t vs = 0;
            assure(vs = jssy_dump_size(obj->subval)); //object cannot be of size 0
            ret += vs;
        }
            break;
#ifdef HAVE_JSSY_BOOL
        case JSSY_BOOL:
        {
            if(obj->boolval){
                ret = 4;//true
            } else{
                ret = 5;//false
            }
        }
            break;
#endif
            
        case JSSY_UNDEFINED:
            break;
    }
    
error:
    if (err) {
        return 0;
    }
    return ret;
}

size_t jssy_dump_internal(jssy_create_tok_t obj, char *buf, size_t bufsize){
    int err = 0;
    size_t realprint = 0;
    switch (obj->type) {
        case JSSY_PRIMITIVE:
            assure(bufsize>=PRIMITIVE_PRINT_LIMIT);
            if (round(obj->numval) == obj->numval) {
                realprint = snprintf(buf, PRIMITIVE_PRINT_LIMIT+1, "%ld",(long)obj->numval);
                assure(realprint < bufsize);
            }else{
                realprint = snprintf(buf, PRIMITIVE_PRINT_LIMIT+1, "%f",obj->numval);
                if (realprint >= bufsize) {
                    realprint = bufsize;
                    char *dotloc = strchr(buf, '.');
                    assure(dotloc);
                    //cutting double is only allowed when we cut after decimal point
                    
                    if (dotloc[1] == '\0') {
                        dotloc[0] = '\0';
                        realprint--;
                    }
                }
            }
            break;
        case JSSY_STRING:
            realprint = snprintf(buf,bufsize+1,"\"%.*s\"",(int)obj->size,obj->value);
            assure(realprint <= bufsize);
            break;
        case JSSY_ARRAY:
        {
            size_t origsize = bufsize;
            assure(bufsize>=2);
            *buf++ = '['; bufsize--;
            jssy_create_tok_t v = obj->subval;
            for (size_t i = 0; i<obj->size; i++,v = v->next) {
                size_t didprint = 0;
                assure(didprint = jssy_dump_internal(v, buf, bufsize));
                buf+=didprint; bufsize-=didprint;
                assure(bufsize>0);
                *buf++ = ',';bufsize--;
            }
            if (buf[-1] == ','){
                buf[-1] = ']';
            }else{
                *buf++ = ']'; bufsize--;
            }
            realprint = origsize-bufsize;
        }
            break;
        case JSSY_DICT:
        {
            size_t origsize = bufsize;
            assure(bufsize>=2);
            *buf++ = '{'; bufsize--;
            jssy_create_tok_t v = obj->subval;
            for (size_t i = 0; i<obj->size; i++,v = v->next) {
                size_t didprint = 0;
                assure(didprint = jssy_dump_internal(v, buf, bufsize));
                buf+=didprint; bufsize-=didprint;
                assure(bufsize>0);
                *buf++ = ',';bufsize--;
            }
            if (buf[-1] == ','){
                buf[-1] = '}';
            }else{
                *buf++ = '}'; bufsize--;
            }
            realprint = origsize-bufsize;
        }
            break;
        case JSSY_DICT_KEY:
        {
            size_t origsize = bufsize;
            realprint = snprintf(buf,bufsize+1,"\"%.*s\"",(int)obj->size,obj->value);
            assure(realprint <= bufsize);
            buf+=realprint; bufsize-=realprint;
            assure(bufsize>0);
            *buf++ = ':';bufsize--;
            assure(realprint = jssy_dump_internal(obj->subval, buf, bufsize));
            realprint += origsize-bufsize;
        }
            break;
#ifdef HAVE_JSSY_BOOL
        case JSSY_BOOL:
        {
            if(obj->boolval){
                assure(bufsize>=4);
                realprint = snprintf(buf, bufsize+1, "true");
            } else{
                assure(bufsize>=5);
                realprint = snprintf(buf, bufsize+1, "false");
            }
        }
            break;
#endif
        case JSSY_UNDEFINED: //THIS IS AN ERROR!
            assure(0);
    }
error:
    if (err) {
        realprint = 0;
    }
    return realprint;
}

char *jssy_dump(jssy_create_tok_t obj){
    char *ret = NULL;
    size_t retsize = 0;
    if ((retsize = jssy_dump_size(obj))) {
        ret = (char*)malloc(retsize+1);
        size_t realprint = 0;
        if ((realprint = jssy_dump_internal(obj, ret, retsize)) && realprint <= retsize) {
            ret[realprint] = '\0';
        } else {
            //error
            safeFree(ret);
            return NULL;
        }
    }
    return ret;
}

void jssy_free(jssy_create_tok_t obj){
    int err = 0;
    switch (obj->type) {
#ifdef HAVE_JSSY_BOOL
        case JSSY_BOOL: //intentionally fall through
#endif
        case JSSY_PRIMITIVE:
            //noting to do here
            break;
        case JSSY_DICT_KEY:
            jssy_free(obj->subval);
        case JSSY_STRING:
            safeFree(obj->value);
            break;

        case JSSY_ARRAY:
            while (obj->size) {
                jssy_array_remove_item(obj, 0);
            }
            break;
        case JSSY_DICT:
            while (obj->size) {
                jssy_dict_remove_item(obj, obj->subval->value);
            }
        break;
        case JSSY_UNDEFINED: //THIS IS AN ERROR!
            assure(0);
    }
    safeFree(obj);
error:
    if (err) {
        return;
    }
}

jssy_create_tok_t jssy_new_string(const char *str){
    int err = 0;
    jssy_create_tok_t ret = NULL;
    
    assure(ret = jssy_new_tok(JSSY_STRING));
    ret->size = strlen(str);
    assure(ret->value = (char*)malloc(ret->size+1));
    strncpy(ret->value, str, ret->size);
    ret->value[ret->size] = '\0';
    
error:
    if (err) {
        if (ret) {
            safeFree(ret->value);
            safeFree(ret);
        }
        return NULL;
    }
    return ret;
}

jssy_create_tok_t jssy_new_primitive(double numval){
    jssy_create_tok_t ret = NULL;
    if ((ret = jssy_new_tok(JSSY_PRIMITIVE))){
        ret->numval = numval;
    }
    return ret;
}

jssy_create_tok_t jssy_new_array(void){
    return jssy_new_tok(JSSY_ARRAY);
}

jssy_create_tok_t jssy_new_dict(void){
    return jssy_new_tok(JSSY_DICT);
}

#ifdef HAVE_JSSY_BOOL
jssy_create_tok_t jssy_new_bool(int value){
    jssy_create_tok_t ret = NULL;
    if((ret = jssy_new_tok(JSSY_BOOL))){
        if(value == 0){
            ret->boolval = 0;
        } else{
            ret->boolval = 1;
        }
    }
    return ret;
}
#endif

#pragma mark interact with array objects

size_t jssy_array_get_size(const jssy_create_tok_t array){
    return (array->type == JSSY_ARRAY) ? array->size : 0;
}

const jssy_create_tok_t jssy_array_get_item(const jssy_create_tok_t array, uint32_t n){
    int err = 0;
    jssy_create_tok_t ret = NULL;
    if (array->type == JSSY_ARRAY) {
        assure(array->size);
        ret = array->subval;
        for (uint32_t i = 1; i<=n; i++) {
            assure(i<array->size);
            ret = ret->next;
        }
    }
error:
    if (err) {
        return NULL;
    }
    return ret;
}

void jssy_array_append_item(jssy_create_tok_t array, jssy_create_tok_t item){
    int err = 0;
    if (array->type == JSSY_ARRAY) {
        assure(array->size+1); //check for array space
        
        assure(!item->prev && !item->next);

        if (!array->subval) {
            array->subval = item;
            item->prev = item->next = item;
        }else{
            jssy_create_tok_t v = NULL;
            assure(v = array->subval);
            assure(v = v->prev);
            item->next = v->next;
            item->next->prev = item;
            item->prev = v;
            v->next = item;
        }
        array->size++;
    }
error:
    if (err) {
        return;
    }
}

void jssy_array_insert_item(jssy_create_tok_t array, jssy_create_tok_t item, uint32_t n){
    int err = 0;
    if (array->type == JSSY_ARRAY) {
        if (array->size == n) {
            jssy_array_append_item(array, item);
        }else{
            jssy_create_tok_t v = NULL;
            assure(array->size+1); //check for array space
            
            assure(!item->prev && !item->next);
            if (n == 0) {
                assure(v = array->subval);
                
                array->subval = item;
                
                item->next = v;
                item->prev = v->prev;
                v->prev = item;

            }else{
                assure(v = jssy_array_get_item(array,n-1));
                item->next = v->next;
                item->next->prev = item;
                item->prev = v;
                v->next = item;
            }
            
            array->size++;
        }
    }
error:
    if (err) {
        return;
    }
}

void jssy_array_remove_item(jssy_create_tok_t array, uint32_t n){
    int err = 0;
    jssy_create_tok_t ret = NULL;
    if (array->type == JSSY_ARRAY) {
        assure(array->size);
        ret = array->subval;
        
        if (array->size == 1) {
            array->subval = NULL;
        }else{
            //2 or more elements
            if (n==0) {
                array->subval = ret->next;
            }else{
                for (uint32_t i = 1; i<=n; i++) {
                    assure(i<array->size);
                    ret = ret->next;
                }
            }
            //we have the item to be removed here
            ret->prev->next = ret->next;
            ret->next->prev = ret->prev;
            
        }
        ret->next = ret->prev = NULL;
        array->size--;
        safeFreeCustom(ret,jssy_free);
    }
error:
    if (err) {
        return;
    }
}

#pragma mark interact with dict objects

size_t jssy_dict_get_size(const jssy_create_tok_t dict){
    return (dict->type == JSSY_DICT) ? dict->size : 0;
}

const jssy_create_tok_t jssy_dict_get_item(const jssy_create_tok_t dict, const char *key){
    if (dict->type == JSSY_DICT) {
        jssy_create_tok_t keys = dict->subval;
        
        for (size_t i = 0; i<dict->size; keys = keys->next,i++) {
            if (strlen(key) == keys->size && strncmp(key, keys->value, keys->size) == 0)
                return keys->subval;
        }
    }
    return NULL;
}

void jssy_dict_set_item(jssy_create_tok_t dict, const char* key, jssy_create_tok_t item){
    int err = 0;
    jssy_create_tok_t dict_key = NULL;
    if (dict->type == JSSY_DICT) {
        assure(dict->size+1); //make sure there is space for another element
        
        assure(dict_key = jssy_new_tok(JSSY_DICT_KEY));
        dict_key->size = strlen(key);
        assure(dict_key->value = (char*)malloc(dict_key->size+1));
        strncpy(dict_key->value, key, dict_key->size+1);
        
        if (dict->size == 0) {
            dict_key->next = dict_key->prev = dict_key;
        }else{
            jssy_create_tok_t v = NULL;
            assure(v = dict->subval);

            dict_key->next = v;
            dict_key->prev = v->prev;
            v->prev = dict_key;
            dict_key->prev->next = dict_key;
        }
        dict->subval = dict_key;
        dict_key->subval = item;
        
        dict->size++;
    }

error:
    if (err) {
        if (dict_key) {
            safeFree(dict_key->value);
            safeFree(dict_key);
        }
        return;
    }
}

void jssy_dict_remove_item(jssy_create_tok_t dict, const char* key){
    if (dict->type == JSSY_DICT) {
        jssy_create_tok_t keys = dict->subval;
        
        for (size_t i = 0; i<dict->size; keys = keys->next,i++) {
            if (strlen(key) == keys->size && strncmp(key, keys->value, keys->size) == 0){
                if (dict->size == 1) {
                    dict->subval = NULL;
                }else if (dict->subval == keys){
                    dict->subval = keys->next;
                }
                //we have the item to be removed here
                keys->prev->next = keys->next;
                keys->next->prev = keys->prev;

                keys->next = keys->prev = NULL;
                dict->size--;
                safeFreeCustom(keys, jssy_free);
                break;
            }
        }
    }
}
