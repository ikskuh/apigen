#include "apigen.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void APIGEN_NORETURN apigen_panic(char const *msg)
{
    (void)fprintf(stderr, "\r\nAPIGEN PANIC: %s\r\n\r\n", msg);
    (void)fflush(stderr);
    exit(1);
}

bool apigen_streq(char const * str1, char const * str2)
{
    APIGEN_NOT_NULL(str1);
    APIGEN_NOT_NULL(str2);

    size_t const str1_len = strlen(str1);
    size_t const str2_len = strlen(str2);
    if(str1_len != str2_len) {
        return false;
    }
    for(size_t i = 0; i < str1_len; i++) {
        if(str1[i] != str2[i]) {
            return false;
        }
    }
    return true;
}