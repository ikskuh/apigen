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
    struct apigen_memory_arena central_arena;
    apigen_memory_arena_init(&central_arena);

    if(argc == 3 && !strcmp(argv[1], "--parser-test")) {

        FILE * f = fopen(argv[2], "rb");
        if(f == NULL) {
            fprintf(stderr, "error: %s not found!\n", argv[2]);
            return 1;
        }

        struct apigen_parser_state state = {
            .file = f,
            .file_name = argv[2],
            .ast_arena = &central_arena,
            .line_feed = "\r\n",
        };

        int result = apigen_parse(&state);

        fclose(f);

        apigen_memory_arena_deinit(&central_arena);
        return result;
    }

    // apigen_generator_renderType(&apigen_gen_zig, &(struct apigen_Type){.id = apigen_typeid_struct}, NULL,
    //                             apigen_io_writeStdOut);
    // apigen_io_writeStdOut(NULL, "\r\n", 2);

    struct apigen_parser_state state = {
        .file = stdin,
        .file_name = "stdin",
        .ast_arena = &central_arena,
        .line_feed = "\r\n",
    };

    int foo = apigen_parse(&state);
    fprintf(stderr, "lex result: %d\n", foo);

    apigen_memory_arena_deinit(&central_arena);
    return 0;
}
