//
//  jssy.h
//  jssy
//
//  Created by tihmstar on 04.04.17.
//  Copyright © 2017 tihmstar. All rights reserved.
//

#ifndef jssy_h
#define jssy_h

#include <stddef.h>

typedef enum {
    /**
     * JSON type identifier. Basic types are:
     * 	o Object
     * 	o Array
     * 	o String
     * 	o Other primitive: number, boolean (true/false) or null
     */
    JSSY_UNDEFINED = 0,
    JSSY_DICT = 1,
    JSSY_DICT_KEY = 2,
    JSSY_ARRAY = 3,
    JSSY_STRING = 4,
    JSSY_PRIMITIVE = 5,
#ifdef HAVE_JSSY_BOOL
    JSSY_BOOL
#endif
} jssytype_t;

enum jssyerr {
    /* Not enough tokens were provided */
    JSSY_ERROR_NOMEM = -1,
    /* Invalid character inside JSON string */
    JSSY_ERROR_INVAL = -2,
    /* The string is not a full JSON packet, more bytes expected */
    JSSY_ERROR_PART = -3
};

typedef struct jssytok{
    jssytype_t type;
    size_t size;
    union{
        char *value;
        double numval;
#ifdef HAVE_JSSY_BOOL
        unsigned char boolval;
#endif
    };
    struct jssytok *subval;
    struct jssytok *next;
    struct jssytok *prev;
} jssytok_t;

long jssy_parse(const char *buffer, size_t bufferSize, jssytok_t *tokens, size_t tockensBufSize);
/*
 run this with tokens=NULL and tokensBufSize=NULL to get the total amount of tokens
 Then alloc tokenCnt*sizeof(jssytype_t) bytes of memory and pass it to tokens and the size to tokensBufSize
 */

jssytok_t *jssy_dictGetValueForKey(const jssytok_t *dict, const char *key);
jssytok_t *jssy_objectAtIndex(const jssytok_t *arr, unsigned index);
int jssy_doForValuesInArray(jssytok_t *arr, int (*func)(jssytok_t *t));


#endif /* jssy_h */
