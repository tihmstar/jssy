//
//  jssy_create.h
//  jssy
//
//  Created by tihmstar on 07.12.20.
//  Copyright © 2020 tihmstar. All rights reserved.
//

#ifndef jssy_create_h
#define jssy_create_h

#include "jssy.h"
#include <stdint.h>

typedef struct jssy_create_tok_private_t * jssy_create_tok_t;

#pragma mark general

char *jssy_dump(jssy_create_tok_t obj);

void jssy_free(jssy_create_tok_t obj);

jssy_create_tok_t jssy_new_string(const char *str);

jssy_create_tok_t jssy_new_primitive(double numval);

jssy_create_tok_t jssy_new_array(void);

jssy_create_tok_t jssy_new_dict(void);

#ifdef HAVE_JSSY_BOOL
jssy_create_tok_t jssy_new_bool(int val);
#endif

#pragma mark interact with array objects

size_t jssy_array_get_size(const jssy_create_tok_t array);

const jssy_create_tok_t jssy_array_get_item(const jssy_create_tok_t array, uint32_t n);

void jssy_array_append_item(jssy_create_tok_t array, jssy_create_tok_t item);

void jssy_array_insert_item(jssy_create_tok_t array, jssy_create_tok_t item, uint32_t n);

void jssy_array_remove_item(jssy_create_tok_t array, uint32_t n);

#pragma mark interact with dict objects

size_t jssy_dict_get_size(const jssy_create_tok_t dict);

const jssy_create_tok_t jssy_dict_get_item(const jssy_create_tok_t dict, const char *key);

void jssy_dict_set_item(jssy_create_tok_t dict, const char* key, jssy_create_tok_t item);

void jssy_dict_remove_item(jssy_create_tok_t dict, const char* key);


#endif /* jssy_create_h */
