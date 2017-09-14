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

int cmain(int argc, const char * argv[]) {
    char *buf = NULL;
    size_t size = 0;
    FILE *f = fopen("offsets.json","r");// place https://api.ipsw.me/v2.1/firmware.json here
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
    
    
    
    return 1;
    
    jssytok_t *devs = jssy_dictGetValueForKey(tokens, "devices");
    
    
    
    printf("done\n");
    free(tokens);
    free(buf);
    return 0;
}
