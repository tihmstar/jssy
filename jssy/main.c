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

int main(int argc, const char * argv[]) {
    char *buf = NULL;
    size_t size = 0;
    FILE *f = fopen("file.json","r");// place https://api.ipsw.me/v2.1/firmware.json here
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    buf = malloc(size);
    fread(buf, 1, size, f);
    fclose(f);
    
    
    
    long ret;
    ret = jssy_parse(buf, size, NULL, 0);
    size_t tokensSize = ret * sizeof(jssytok_t);
    jssytok_t *tokens = malloc(tokensSize);
    
    ret = jssy_parse(buf, size, tokens, tokensSize);
    printf("ret=%d\n",ret);
    
    jssytok_t *devs = jssy_dictGetValueForKey(tokens, "devices");
    if (!devs) devs = tokens;
    jssy_doForValuesInArray(devs, {
        printf("%.*s ----------------------------\n",t->size,t->value);
        jssytok_t *firmwares = jssy_dictGetValueForKey(t->subval,"firmwares");
        jssy_doForValuesInArray(firmwares, {
            jssytok_t *url = jssy_dictGetValueForKey(t, "buildid");
            printf("%.*s, ",url->size,url->value);
        });
        printf("\n");
    
    });
    
    
    
    printf("done\n");
    free(tokens);
    free(buf);
    return 0;
}
