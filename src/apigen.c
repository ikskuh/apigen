#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apigen.h"

#define INVALIDATE_VALUE(_X) memset(&_X, 0xAA, sizeof(_X))

void apigen_type_free(struct apigen_Type *type)
{
    if (type->extra != NULL)
    {
        // TODO: Recursively destroy types
        free(type->extra);
    }
    INVALIDATE_VALUE(*type);
}

void apigen_generator_renderType(struct apigen_Generator const *generator, struct apigen_Type const *type,
                                 apigen_Stream stream, apigen_StreamWriter writer)
{
    generator->render_type(generator, type, stream, writer);
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    apigen_generator_renderType(&apigen_gen_zig, &(struct apigen_Type){.id = apigen_typeid_struct}, NULL,
                                apigen_io_writeStdOut);
    apigen_io_writeStdOut(NULL, "\r\n", 2);

    return 0;
}

void APIGEN_NORETURN apigen_panic(char const *msg)
{
    (void)fprintf(stderr, "\r\nAPIGEN PANIC: %s\r\n\r\n", msg);
    (void)fflush(stderr);
    exit(1);
}
