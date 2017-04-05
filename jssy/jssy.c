//
//  jssy.c
//  jssy
//
//  Created by tihmstar on 04.04.17.
//  Copyright © 2017 tihmstar. All rights reserved.
//

#include "jssy.h"

#define clearTok(tok) tok->type = JSSY_UNDEFINED, tok->size=0, tok->value = NULL, tok->next = NULL, tok->prev = NULL
#define assure(code,cond) do {if (!(cond)){*bufferSize = 0; return code; }} while(0)
#define incBuf (*bufferSize ? (--*bufferSize,++*buffer) : (ret=JSSY_ERROR_PART ,(char*)0))
#define valIncBuf (*bufferSize ? (--*bufferSize,*(*buffer)++) : (ret=JSSY_ERROR_PART, 0))
#define decBuf (++*bufferSize,--*buffer)
#define incTok (*tokens ? (*tokensBufSize ? (++*tokens) : (ret=JSSY_ERROR_PART ,(jssytok_t*)0)) : 0 )

static inline int isBlank(char c){
    return c == ' ' || c == '\n' || c == '\r';
}

long jssy_parse_p(char **buffer, size_t *bufferSize, jssytok_t **tokens, size_t *tokensBufSize){
    long ret = 1;
    long nowParse = 0;
    char c = 0;
    jssytok_t tmp;
    jssytok_t *tmplink;
    
    assure(JSSY_ERROR_PART, bufferSize);
    if (*tokens)
        assure(JSSY_ERROR_NOMEM, *tokensBufSize >= sizeof(jssytok_t));
    *tokensBufSize -= sizeof(jssytok_t);
    jssytok_t *tk = *tokens ? *tokens : &tmp;
    clearTok(tk);
    while ((c = valIncBuf) && isBlank(c));
    switch (c) {
        case '[':
            tk->type = JSSY_ARRAY;
            tk->subval = &(*tokens)[1];
        reparseArr:
            tmplink = incTok;
            nowParse = jssy_parse_p(buffer, bufferSize, tokens, tokensBufSize);
            ret += nowParse;
            tk->size += nowParse;
            while ((c = valIncBuf) && isBlank(c));
            if (c == ','){
                if (*tokens) tmplink->next = &(*tokens)[1];
                goto reparseArr;
            }else if (c == ']'){
                if (*tokens) {
                    tk = tk->subval;
                    tk->prev = tmplink;
                    tmplink->next = tk;
                    while (tk != tmplink) {
                        tk->next->prev = tk;
                        tk = tk->next;
                    }
                }
                return ret;
            }else
                assure(JSSY_ERROR_INVAL, 0);
            break;
        case '{':
            tk->type = JSSY_DICT;
            tk->subval = &(*tokens)[1];
        reparseObject:
            tmplink = incTok;
            nowParse = jssy_parse_p(buffer, bufferSize, tokens, tokensBufSize);
            if (*tokens) tmplink->type = JSSY_DICT_KEY;
            ret += nowParse;
            tk->size += nowParse;
            while ((c = valIncBuf) && isBlank(c));
            assure(JSSY_ERROR_INVAL,c == ':');
            if (*tokens) tmplink->subval = incTok;
            nowParse = jssy_parse_p(buffer, bufferSize, tokens, tokensBufSize);
            ret +=nowParse;
            tk->size += nowParse;
            while ((c = valIncBuf) && isBlank(c));
            if (c == ','){
                if (*tokens) tmplink->next = &(*tokens)[1];
                goto reparseObject;
            }else if (c == '}'){
                if (*tokens) {
                    tk = tk->subval;
                    tk->prev = tmplink;
                    tmplink->next = tk;
                    while (tk != tmplink) {
                        tk->next->prev = tk;
                        tk = tk->next;
                    }
                }
                return ret;
            }else
                assure(JSSY_ERROR_INVAL, 0);
            break;
        case '"':
            tk->type = JSSY_STRING;
            tk->value = *buffer;
            while ((c = valIncBuf) && c != '"')
                tk->size++;
            return 1;
        case 't':
        case 'T':
            assure(JSSY_ERROR_INVAL, (c = valIncBuf) && c == 'r' || c == 'R');
            assure(JSSY_ERROR_INVAL, (c = valIncBuf) && c == 'u' || c == 'U');
            assure(JSSY_ERROR_INVAL, (c = valIncBuf) && c == 'e' || c == 'E');
            tk->type = JSSY_PRIMITIVE;
            tk->numval = 1;
            return 1;
        case 'f':
        case 'F':
            assure(JSSY_ERROR_INVAL, (c = valIncBuf) && c == 'a' || c == 'A');
            assure(JSSY_ERROR_INVAL, (c = valIncBuf) && c == 'l' || c == 'L');
            assure(JSSY_ERROR_INVAL, (c = valIncBuf) && c == 's' || c == 'S');
            assure(JSSY_ERROR_INVAL, (c = valIncBuf) && c == 'e' || c == 'E');
            tk->type = JSSY_PRIMITIVE;
            tk->numval = 0;
            return 1;
        case '-':
            nowParse = 1;
            incBuf;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            tk->type = JSSY_PRIMITIVE;
            do {
                tk->numval *= 10;
                tk->numval += c - '0';
            } while ((c = valIncBuf) && c >= '0' && c <= '9');
            if (nowParse)
                tk->numval = -tk->numval;
            decBuf;
            return 1;
            
        default:
            assure(JSSY_ERROR_INVAL, 0);
            break;
    }
    
    return ret;
}

long jssy_parse(char *buffer, size_t bufferSize, jssytok_t *tokens, size_t tockensBufSize){
    return jssy_parse_p(&buffer, &bufferSize, &tokens, &tockensBufSize);
}

#pragma mark helpers

static inline int m_strcmp(const char *a, const char *b, size_t s){
    char c = 0;
    while (s-- && *a && *b && !(c = *a++ - *b++));
    return (int)c;
}

jssytok_t *jssy_dictGetValueForKey(const jssytok_t *dict, const char *key){
    if (dict->type != JSSY_DICT)
        return NULL;
    jssytok_t *t = dict->subval;
    for (size_t i = 0; i<dict->size; t = t->next,i++) {
        if (m_strcmp(key, t->value, t->size) == 0)
            return t->subval;
    }
    return NULL;
}

jssytok_t *jssy_objectAtIndex(const jssytok_t *arr, unsigned index){
    if ((arr->type != JSSY_DICT && arr->type != JSSY_ARRAY) || index >= arr->size)
        return NULL;
    while (index--) arr = arr->next;
    return (jssytok_t *)arr;
}
