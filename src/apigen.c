#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apigen.h"

#define INVALIDATE_VALUE(_X) memset(&_X, 0xAA, sizeof(_X))

int main(int argc, char **argv)
{
    struct apigen_MemoryArena central_arena;
    apigen_memory_arena_init(&central_arena);

    if(argc == 3 && apigen_streq(argv[1], "--parser-test")) {
        FILE * f = fopen(argv[2], "rb");
        if(f == NULL) {
            fprintf(stderr, "error: %s not found!\n", argv[2]);
            return 1;
        }

        struct apigen_ParserState state = {
            .file = f,
            .file_name = argv[2],
            .ast_arena = &central_arena,
            .line_feed = "\r\n",
        };

        bool parse_ok = apigen_parse(&state);

        fclose(f);

        apigen_memory_arena_deinit(&central_arena);
        return parse_ok ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    // apigen_generator_renderType(&apigen_gen_zig, &(struct apigen_Type){.id = apigen_typeid_struct}, NULL,
    //                             apigen_io_writeStdOut);
    // apigen_io_writeStdOut(NULL, "\r\n", 2);

    struct apigen_ParserState state = {
        .file = stdin,
        .file_name = "stdin",
        .ast_arena = &central_arena,
        .line_feed = "\r\n",
    };

    bool parse_ok = apigen_parse(&state);
    if(parse_ok) {
        fprintf(stderr, "lex ok.\n");

        struct apigen_Document document;

        bool analyze_ok = apigen_analyze(&state, &document);
        if(analyze_ok) {
            fprintf(stderr, "analyze ok.\n");
        }
        else {
            fprintf(stderr, "analyze failed!\n");
        }
    }
    else {
        fprintf(stderr, "lex failed!\n");
    }

    apigen_memory_arena_deinit(&central_arena);
    return parse_ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
