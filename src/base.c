#include "apigen.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void APIGEN_NORETURN apigen_panic(char const *msg)
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
