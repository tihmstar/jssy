//
//  jssy.c
//  jssy
//
//  Created by tihmstar on 04.04.17.
//  Copyright © 2017 tihmstar. All rights reserved.
//

#include "jssy.h"

#define clearTok(tok) tok->type = JSSY_UNDEFINED, tok->size=0, tok->value = NULL, tok->subval = NULL, tok->next = NULL, tok->prev = NULL
#define assure(code,cond) do {if (!(cond)){bufferSize = 0; return code; }} while(0)
#define valIncBuf (bufferSize ? (--bufferSize,*buffer++) : (ret=(tokens ? JSSY_ERROR_PART : ret), 0))
#define decBuf (++bufferSize,--buffer)
#define incTok (tokens ? (tokensBufSize >= sizeof(jssytok_t) ? (++ret,tokensBufSize-=sizeof(jssytok_t),tokens++) : (ret=JSSY_ERROR_NOMEM ,(jssytok_t*)0)) : (++ret,&deadToken))
#define nextTok (tokens ? (tokensBufSize >= sizeof(jssytok_t) ? tokens : (ret=JSSY_ERROR_NOMEM ,(jssytok_t*)0)) : &deadToken)

#define linkBasicElem if (!cur->next) return ret;assure(JSSY_ERROR_INVAL, cur->prev)
/*                     -----------(1)------------;-----------------(2)---------------
(1) check if this is the whole json, or if token is in subcontainer
(2) token is in subcontainer. Check if token was linked correctly. Is a "," or a ":" missing?
*/

static inline int isBlank(char c){
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

long jssy_parse(const char *buffer, size_t bufferSize, jssytok_t *tokens, size_t tokensBufSize){
    jssytok_t *cur = NULL;
    jssytok_t *tmp = NULL;
    long ret = 0;
    jssytok_t deadToken = {JSSY_UNDEFINED,1234567890};
    char c = '\0';
 
    if (!tokensBufSize)
        tokens = NULL;
    else{
        assure(JSSY_ERROR_NOMEM, tokens);
        //cleartokens
        cur = tokens;
        for (size_t i = tokensBufSize; i>=sizeof(jssytok_t); i-=sizeof(jssytok_t)) {
            clearTok(cur);
            cur++;
        }
    }
    
    cur = &deadToken;
    
doparse:
    {
        long nowParse = 0;
        while ((c = valIncBuf) && isBlank(c)){}
        if (cur->next && (cur->next->type == JSSY_ARRAY || cur->next->type == JSSY_DICT))
            cur->next->size++;
        
        switch (c) {
            case '[':
            case '{':
                assure(JSSY_ERROR_NOMEM, cur = incTok);
                cur->type = (c == '[') ? JSSY_ARRAY : JSSY_DICT;
                assure(JSSY_ERROR_INVAL,cur->subval = nextTok);
                cur->subval->next = cur->subval->prev = cur;
                
                deadToken.next = cur;//this is required to detect special case where array is empty
                cur = &deadToken;
                break;
            case ':':
                if (!tokens) break;
                assure(JSSY_ERROR_INVAL, cur->type == JSSY_DICT_KEY);
                assure(JSSY_ERROR_INVAL, cur->subval = nextTok);
                cur->subval->next = cur->subval->prev = cur;
                break;
            case ',':
                if (!tokens) break;
                assure(JSSY_ERROR_INVAL, cur && cur->next); //check the container of the token
                if (cur->next->type == JSSY_ARRAY) {
                    tmp = cur->next; //backup owner
                }else if (cur->next->type == JSSY_DICT_KEY){
                    tmp = cur->next->next; //backup owner
                    cur->next = cur->prev = NULL;
                    cur = cur-1; //the previus token has to be the matching key to this value. No other memory layout possible
                }else
                    assure(JSSY_ERROR_INVAL, 0);
                assure(JSSY_ERROR_INVAL, cur->next = nextTok);
                cur->next->prev = cur;
                cur->next->next = tmp;
                tmp = NULL;
                break;
            case ']':
            case '}':
                if (!tokens) break;
                assure(JSSY_ERROR_INVAL, cur && cur->next); //check the container of the token
                if (c == ']')
                    assure(JSSY_ERROR_INVAL, cur->next->type == JSSY_ARRAY);
                else{
                    if (cur == &deadToken) {
                        //handle special case when dict is empty
                        cur = deadToken.next;
                        cur->size = 0;
                        cur->subval = 0;
                        deadToken.next = NULL;
                        goto checklast;
                    }else{
                        assure(JSSY_ERROR_INVAL, cur->next->type == JSSY_DICT_KEY);
                        cur->next = cur->prev = NULL;
                        cur = cur-1; //the previus token has to be the matching key to this value. No other memory layout possible
                    }
                }
                
                if (!--cur->next->size){ //correct array size
                    cur = cur->next;
                    cur->subval = NULL; //array is empty
                }else{
                    //fixup linking
                    cur->next->subval->prev = cur; //make firstelem->prev point to this (last object)
                    cur = cur->next; //make cur be the container again
                    cur->subval->prev->next = cur->subval; //make lastelem->next point to first object
                }
            checklast:
                if (!cur->prev)
                    return ret; //i am the last object
                break;
            case '"':
                assure(JSSY_ERROR_NOMEM, cur = incTok);
                cur->type = JSSY_STRING;
                cur->value = (char*)buffer;
                while ((c = valIncBuf) && (nowParse++ || c != '"')){
                    cur->size++;
                    if (c != '\\' || nowParse == 2)
                        nowParse = 0;
                }
                
                if (!tokens) break;
                linkBasicElem;
                if (cur->next->type == JSSY_DICT) // if token is owned by a dict, it is a dict_key instead of a string
                    cur->type = JSSY_DICT_KEY;
                break;
            case 't':
                assure(JSSY_ERROR_NOMEM, cur = incTok);
                assure(JSSY_ERROR_INVAL, (c = valIncBuf) && c == 'r');
                assure(JSSY_ERROR_INVAL, (c = valIncBuf) && c == 'u');
                assure(JSSY_ERROR_INVAL, (c = valIncBuf) && c == 'e');
                cur->type = JSSY_PRIMITIVE;
                cur->numval = 1;
                
                linkBasicElem;
                break;
            case 'n':
                assure(JSSY_ERROR_INVAL, (c = valIncBuf) && c == 'u');
                assure(JSSY_ERROR_INVAL, (c = valIncBuf) && c == 'l');
                assure(JSSY_ERROR_INVAL, (c = valIncBuf) && c == 'l');
                goto isfalse;
            case 'f':
                assure(JSSY_ERROR_INVAL, (c = valIncBuf) && c == 'a');
                assure(JSSY_ERROR_INVAL, (c = valIncBuf) && c == 'l');
                assure(JSSY_ERROR_INVAL, (c = valIncBuf) && c == 's');
                assure(JSSY_ERROR_INVAL, (c = valIncBuf) && c == 'e');
            isfalse:
                assure(JSSY_ERROR_NOMEM, cur = incTok);
                cur->type = JSSY_PRIMITIVE;
                cur->numval = 0;
                
                linkBasicElem;
                break;
            case '-':
                nowParse = 1;
                assure(JSSY_ERROR_INVAL, (c = valIncBuf) && c >= '0' && c <= '9');
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '0':
                assure(JSSY_ERROR_NOMEM, cur = incTok);
                if (c == '0') goto caseZero;
                do {
                    cur->numval *= 10;
                    cur->numval += c - '0';
                } while ((c = valIncBuf) && c >= '0' && c <= '9');
                goto parsefloat;
            caseZero:
                c = valIncBuf;
            parsefloat:
                cur->type = JSSY_PRIMITIVE;
                if (c == '.') {
                    assure(JSSY_ERROR_INVAL, (c = valIncBuf) && c >= '0' && c <= '9');
                    float fhelper = 0.1;
                    do {
                        cur->numval += (c - '0') * fhelper;
                        fhelper /= 10;
                    } while ((c = valIncBuf) && c >= '0' && c <= '9');
                }
                if (c == 'e' || c == 'E') {
                    char sign;
                    assure(JSSY_ERROR_INVAL, (sign = valIncBuf) && (sign == '-' || sign == '+'));
                    assure(JSSY_ERROR_INVAL, (c = valIncBuf) && c >= '0' && c <= '9');
                    int ehelper = 0;
                    do {
                        ehelper *=10;
                        ehelper += c - '0';
                    } while ((c = valIncBuf) && c >= '0' && c <= '9');
                    while (ehelper--) {
                        if (sign == '+')
                            cur->numval *=10;
                        else
                            cur->numval /= 10;
                    }
                }
                if (nowParse)
                    cur->numval = -cur->numval;
                assure(JSSY_ERROR_INVAL, c == 0 || isBlank(c) || c == ']' || c == '}' || c == ',');
                decBuf;
                linkBasicElem;
                break;
            default:
                if (ret < 0 || !tokens)
                    return ret;
                else{
                    assure(JSSY_ERROR_PART, c && bufferSize);
                    assure(JSSY_ERROR_INVAL, 0);
                }
                break;
        }
    }
    if (tokens || bufferSize)
        goto doparse;
    
    
    return ret;
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
