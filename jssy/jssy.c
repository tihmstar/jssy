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
#define valIncBuf (bufferSize ? (--bufferSize,*buffer++) : (ret=((tokens || isCounterMode) ? JSSY_ERROR_PART : ret), 0))
#define valIncBufUnsafe (--bufferSize,*buffer++)
#define decBuf ((ret>=0) ? (++bufferSize,--buffer) : 0)
#define incTok (tokens ? (tokensBufSize >= sizeof(jssytok_t) ? (++ret,tokensBufSize-=sizeof(jssytok_t),tokens++) : (ret=JSSY_ERROR_NOMEM ,(jssytok_t*)0)) : (++ret,&deadToken))
#define nextTok (tokens ? (tokensBufSize >= sizeof(jssytok_t) ? tokens : ((jssytok_t*)0)) : &deadToken)

#define linkBasicElem {if (!cur->next){ assure(JSSY_ERROR_INVAL, cur == tokens-ret); goto endparse;};assure(JSSY_ERROR_INVAL, cur->prev);}
/*                     -----------(1)------------;-----------------(2)---------------
(1) check if this is the whole json, or if token is in subcontainer
(2) token is in subcontainer. Check if token was linked correctly. Is a "," or a ":" missing?
*/

static inline int isBlank(char c){
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}
static inline int isHex(char c){
    return (('a' <= c && c <= 'f') || ('A' <= c && c <= 'F') || ('0' <= c && c <= '9'));
}

long jssy_parse(const char *buffer, size_t bufferSize, jssytok_t *tokens, size_t tokensBufSize){
    jssytok_t *cur = NULL;
    jssytok_t *tmp = NULL;
    long ret = 0;
    jssytok_t deadToken = {JSSY_UNDEFINED,1234567890};
    int isCounterMode = (tokens == NULL);
    char c = '\0';
    
#if DEBUG
    jssytok_t *startOfTokens = tokens;
#endif
 
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
    
    do {
        long nowParse = 0;
        while (bufferSize && isBlank((c = valIncBufUnsafe))){}
        if (cur->next && (cur->next->type == JSSY_ARRAY || cur->next->type == JSSY_DICT))
            cur->next->size++;
        
        switch (c) {
            case '[':
            case '{':
                assure(JSSY_ERROR_NOMEM, cur = incTok);
                cur->type = (c == '[') ? JSSY_ARRAY : JSSY_DICT;
                if ((cur->subval = nextTok))
                    cur->subval->next = cur->subval->prev = cur;
                
                deadToken.next = cur;//this is required to detect special case where array is empty
                cur = &deadToken;
                break;
            case ':':
                if (isCounterMode) break;
                assure(JSSY_ERROR_INVAL, cur->type == JSSY_DICT_KEY);
                assure(JSSY_ERROR_INVAL, !cur->subval && (cur->subval = nextTok));
                cur->subval->next = cur->subval->prev = cur;
                break;
            case ',':
                if (isCounterMode) break; //in counter mode we don't need to link
                assure(JSSY_ERROR_INVAL, cur && cur->next && cur != &deadToken); //check the container of the token //there is not legit situation where cur can be deadToken when this code is reached
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
                if (isCounterMode) break;
                assure(JSSY_ERROR_INVAL, cur && cur->next); //check the container of the token
                if (cur == &deadToken) {
                    //handle special case when dict is empty
                    cur = deadToken.next;
                    cur->size = 0;
                    assure(JSSY_ERROR_INVAL, !cur->subval); //make sure this really only happens when array/dict was empty
                    deadToken.next = NULL;
                    goto checklast;
                }else if (c == '}'){
                    assure(JSSY_ERROR_INVAL, cur->next->type == JSSY_DICT_KEY);
                    cur->next = cur->prev = NULL;
                    cur = cur-1; //the previus token has to be the matching key to this value. No other memory layout possible
                }else if (c == ']'){
                    //Because who would do such evil things like {"a":1,] ???
                    assure(JSSY_ERROR_INVAL, cur->next->type == JSSY_ARRAY);
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
                assure(JSSY_ERROR_INVAL, (c == '}' && cur->type == JSSY_DICT) || (c == ']' && cur->type == JSSY_ARRAY)); //check closing sequence matches obj type
                if (!cur->prev)
                    goto endparse; //i am the last object
                break;
            case '"':
                assure(JSSY_ERROR_NOMEM, cur = incTok);
                cur->type = JSSY_STRING;
                cur->value = (char*)buffer;
                //use lower 2 bits of nowPares for escaping and next 3 bits for checking \u sequences
                while ((c = valIncBuf) && ((nowParse++) & 1 || c != '"')){
                    cur->size++;
                    assure(JSSY_ERROR_INVAL, c != '\n' && c != '\t');
                    if (nowParse & 2)
                        assure(JSSY_ERROR_INVAL, c == '"' || c == '\\' || c == '/' || c == 'b' || c == 'n' || c == 'f' || c == 'r' || c == 't' || c == 'u');
                    switch (nowParse & (1<<2 | 1 << 3 | 1 << 4)) {
                        case (1<<2 | 1 << 3 | 1 << 4): //4
                            nowParse &= ~(1<<2); //->3
                            assure(JSSY_ERROR_INVAL, isHex(c));
                            break;
                        case (1 << 3 | 1 << 4): //3
                            nowParse &= ~(1<<3); //->2
                            assure(JSSY_ERROR_INVAL, isHex(c));
                            break;
                        case (1 << 4)://2
                            nowParse &= ~(1<<4);
                            nowParse |= (1<<3); //->1
                            assure(JSSY_ERROR_INVAL, isHex(c));
                            break;
                        case (1 << 3):
                            nowParse &= ~(1<<3);
                            assure(JSSY_ERROR_INVAL, isHex(c));
                    }
                    if (c == 'u' && nowParse & 2){
                        assure(JSSY_ERROR_INVAL, !(nowParse &~3)); //new \u sequence starting, but old not finished
                        nowParse |= (1<<2 | 1 << 3 | 1 << 4);
                    }
                    if (c != '\\' || nowParse & 2)
                        nowParse &= ~3;
                }
                assure(JSSY_ERROR_INVAL, nowParse <= 2); //if >2 then \u sequnce not finished
                if (isCounterMode) break;
                linkBasicElem;
                if (cur->next->type == JSSY_DICT){ // if token is owned by a dict, it is a dict_key instead of a string
                    cur->type = JSSY_DICT_KEY;
                    cur->next->size--; //decrement JSSY_DICT size, since otherwise we count both JSSY_DICT_KEY and it's value as elements
                }
                break;
            case 't':
                assure(JSSY_ERROR_NOMEM, cur = incTok);
                assure(JSSY_ERROR_INVAL, (c = valIncBuf) && c == 'r');
                assure(JSSY_ERROR_INVAL, (c = valIncBuf) && c == 'u');
                assure(JSSY_ERROR_INVAL, (c = valIncBuf) && c == 'e');
                if (!isCounterMode) { //check if we are in countermode
#ifdef HAVE_JSSY_BOOL
                    cur->type = JSSY_BOOL;
                    cur->boolval = 1;
#else
                    cur->type = JSSY_PRIMITIVE;
                    cur->numval = 1;
#endif
                    
                    linkBasicElem;
                }
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
                if (!isCounterMode) { //check if we are in countermode
#ifdef HAVE_JSSY_BOOL
                    cur->type = JSSY_BOOL;
                    cur->boolval = 0;
#else
                    cur->type = JSSY_PRIMITIVE;
                    cur->numval = 0;
#endif
                    
                    linkBasicElem;
                }
                break;
            case '-':
                nowParse = 1; //set "negative" bit
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
                    if (!bufferSize){
                        nowParse |= 2; //set reached end of memory bit
                        break;
                    }
                } while ((c = valIncBufUnsafe) && c >= '0' && c <= '9');
                goto parsedouble;
            caseZero:
                if (!bufferSize){
                    nowParse |= 2;
                    c = '\0';
                }else
                    c = valIncBuf;
            parsedouble:
                cur->type = JSSY_PRIMITIVE;
                if (c == '.') {
                    assure(JSSY_ERROR_INVAL, (c = valIncBuf) && c >= '0' && c <= '9');
                    double fhelper = 0.1;
                    do {
                        cur->numval += (c - '0') * fhelper;
                        fhelper /= 10;
                        if (!bufferSize){
                            nowParse |= 2; //set reached end of memory bit
                            break;
                        }
                    } while ((c = valIncBufUnsafe) && c >= '0' && c <= '9');
                }
                if (c == 'e' || c == 'E') {
                    char sign;
                    assure(JSSY_ERROR_INVAL, (sign = valIncBuf));
                    if (sign != '-' && sign != '+'){
                        decBuf;
                        sign = '+';
                    }
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
                if (nowParse & 1) //check negative bit
                    cur->numval = -cur->numval;
                if (!(nowParse & 2)) decBuf; //check "reached end of memory" bit
                if (!isCounterMode){ //only do this in non-counter mode
                    linkBasicElem;
                }
                break;
            default:
                assure(JSSY_ERROR_INVAL, ret > 0);
                assure(JSSY_ERROR_NOMEM, !(!c && bufferSize)); //valid: (no char & no bufSize) and (char & bufsize) and (char and no bufsize) //invalid: (no char & bufsize)
                if (c && !isCounterMode)
                    assure(JSSY_ERROR_INVAL, 0);
        }
    }while (tokens || (bufferSize && isCounterMode));
    
    
endparse:
    while (bufferSize) assure(JSSY_ERROR_INVAL, !(c=valIncBuf) || isBlank(c));
    
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

int jssy_doForValuesInArray(jssytok_t *arr, int (*func)(jssytok_t *t)){
    int rt = 0;
    if ((arr->type == JSSY_DICT || arr->type == JSSY_ARRAY)){
        size_t i=0;
        for (jssytok_t *t=arr->subval; i<arr->size; t=t->next,i++) {
            if ((rt = func(t)))
                return rt;
        }
    }else
        return -1;
    
    return 0;
}
