#include <stdio.h>
#include <stdlib.h>

#include "apigen.h"
#include "apigen-internals.h"

static int apigen_main(
    struct apigen_MemoryArena * const arena,
    struct apigen_Diagnostics * const diagnostics,
    struct CliOptions const * const   options)
{
    struct apigen_ParserState state = {
        .source_dir  = apigen_io_cwd(),
        .file        = apigen_io_null,
        .file_name   = "stdin",
        .ast_arena   = arena,
        .line_feed   = "\r\n",
        .diagnostics = diagnostics,
    };

    if (options->positional_count != 1) {
        fprintf(stderr, "error: apigen requires exactly one positional file!\n");
        return EXIT_FAILURE;
    }

    if(!apigen_open_input_from_cwd(&state, options->positionals[0])) {
        return EXIT_FAILURE;
    }

    if(options->language == LANG_GO)
        apigen_panic("oh no!");

    bool ok = apigen_parse(&state);
    if (ok) {
        struct apigen_Document document;

        ok = apigen_analyze(&state, &document);
        if (ok) {

            FILE * output;
            if ((options->output == NULL) || apigen_streq(options->output, "-")) {
                output = stdout;
            }
            else {
                output = fopen(options->output, "wb");
                if (output == NULL) {
                    fprintf(stderr, "error: could not open %s!\n", options->output);
                    return EXIT_FAILURE;
                }
            }

            struct apigen_Stream out_stream = apigen_io_from_stream(output);

            switch (options->language) {
                case LANG_C:
                    ok = apigen_render_c(out_stream, arena, diagnostics, &document);
                    break;
                case LANG_CPP:
                    ok = apigen_render_cpp(out_stream, arena, diagnostics, &document);
                    break;
                case LANG_ZIG:
                    ok = apigen_render_zig(out_stream, arena, diagnostics, &document);
                    break;
                case LANG_RUST:
                    ok = apigen_render_rust(out_stream, arena, diagnostics, &document);
                    break;
                case LANG_GO:
                    ok = apigen_render_go(out_stream, arena, diagnostics, &document);
                    break;
            }

            if (output != stdout) {
                fclose(output);
            }
        }
    }

    apigen_io_close(&state.file);
    apigen_io_close_dir(&state.source_dir);

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}


static int wrapped_main(
    struct apigen_MemoryArena * const arena,
    struct apigen_Diagnostics * const diagnostics,
    struct CliOptions const * const   options)
{
    if (options->help) {
        apigen_print_help(options->executable, stdout);
        return EXIT_SUCCESS;
    }

    if (options->test_mode == TEST_MODE_DISABLED) {
        return apigen_main(arena, diagnostics, options);
    }
    else {
        return apigen_test_runner(arena, diagnostics, options);
    }
}

int main(int argc, char ** argv)
{
    apigen_enable_debug_diagnostics();

    struct CliOptions options = apigen_parse_options_or_exit(argc, argv);

    struct apigen_MemoryArena central_arena;
    apigen_memory_arena_init(&central_arena);

    struct apigen_Diagnostics diagnostics;
    apigen_diagnostics_init(&diagnostics, &central_arena);

    int result = wrapped_main(&central_arena, &diagnostics, &options);

    apigen_diagnostics_render(&diagnostics, apigen_io_stderr);

    apigen_diagnostics_deinit(&diagnostics);
    apigen_memory_arena_deinit(&central_arena);

    return result;
}

