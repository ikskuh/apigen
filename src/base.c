#include "apigen.h"

#include <stdlib.h>
#include <stdio.h>

void APIGEN_NORETURN apigen_panic(char const *msg)
{
    (void)fprintf(stderr, "\r\nAPIGEN PANIC: %s\r\n\r\n", msg);
    (void)fflush(stderr);
    exit(1);
}
