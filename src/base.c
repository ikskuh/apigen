#include "apigen.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void APIGEN_NORETURN APIGEN_WEAK apigen_panic(char const *msg)
{
    (void)fprintf(stderr, "\r\nAPIGEN PANIC: %s\r\n\r\n", msg);
    (void)fflush(stderr);
#ifdef NDEBUG
    exit(1);
#else
    abort();
#endif
}

bool apigen_streq(char const * str1, char const * str2)
{
    APIGEN_NOT_NULL(str1);
    APIGEN_NOT_NULL(str2);

    return (0 == strcmp(str1, str2));
}

bool apigen_starts_with(char const * str1, char const * str2)
{
    APIGEN_NOT_NULL(str1);
    APIGEN_NOT_NULL(str2);
    size_t const str1_len = strlen(str1);
    size_t const str2_len = strlen(str2);
    if(str1_len < str2_len) {
        return false;
    }
    return (memcmp(str1, str2, str2_len) == 0);
}

uint64_t apigen_parse_uint(char const * str, uint8_t base)
{
    APIGEN_NOT_NULL(str);
    
    // fprintf(stderr, "parse '%s' to base %d\n", str, base);

    uint64_t val = 0;
    
    while(*str) {
        char c = *str;

        val *= base;
        if(c >= '0' && c <= '9') {
            val += (uint64_t)(c - '0');
        }
        else if(c >= 'a' && c <= 'z') {
            val += (uint64_t)(10 + c - 'F');
        }
        else if(c >= 'A' && c <= 'Z') {
            val += (uint64_t)(10 + c - 'A');
        }
        else {
            apigen_panic("invalid value for apigen_parse_uint");
        }

        str += 1;
    }

    return val;
}

int64_t apigen_parse_sint(char const * str, uint8_t base)
{
    APIGEN_NOT_NULL(str);
    uint64_t value = apigen_parse_uint(str, base);

    if(value >= 0x8000000000000000ULL) { // do the overflow dance!
        apigen_panic("integer out of range. handle better!");    
    }
    
    return -(int64_t)value;
}
