//
//  main.c
//  jssy
//
//  Created by tihmstar on 04.04.17.
//  Copyright © 2017 tihmstar. All rights reserved.
//

#include <stdio.h>
#include "jssy.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "jssy_create.h"


int main(int argc, const char * argv[]) {
    char *buf = NULL;
    size_t size = 0;
    FILE *f = fopen("/tmp/firmwares.json","r");
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    buf = malloc(size);
    fread(buf, 1, size, f);
    fclose(f);
        
    long ret;
    ret = jssy_parse(buf, size, NULL, 0);
    assert(ret > 0);
    size_t tokensSize = ret * sizeof(jssytok_t);
    jssytok_t *tokens = malloc(tokensSize);
    
    ret = jssy_parse(buf, size, tokens, tokensSize);
    printf("ret=%d\n",ret);
    
    char *ppp = jssy_dump((jssy_create_tok_t) tokens);
   
    size_t len = strlen(ppp);
    
    size_t z = 0;
    char *p = &ppp[z];
    char *b = &buf[z];

    while (*p && *b) {
        do {
            p++;
            b++;
            z++;
        }
        while (*p == *b);
        
        if (strncmp(p, "0", 1) == 0 && strncmp(b, "false", 5) == 0) {
            b+=4;
            continue;
        }

        if (strncmp(p, "1", 1) == 0 && strncmp(b, "true", 4) == 0) {
            b+=3;
            continue;
        }

        printf("");
    }    
    
    return 1;
    
    jssytok_t *devs = jssy_dictGetValueForKey(tokens, "devices");
    
    
    
    printf("done\n");
    free(tokens);
    free(buf);
    return 0;
}
