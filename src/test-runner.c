#include "apigen.h"
#include "apigen-internals.h"

#include <ctype.h>
#include <stdlib.h>

struct CodeArray
{
    size_t                       len;
    enum apigen_DiagnosticCode * ptr;
};

static char * str_trim(char * string)
{
    APIGEN_NOT_NULL(string);

    while (*string && isspace(*string)) {
        string += 1;
    }

    char * start = string;
    while (*string && !isspace(*string)) {
        string += 1;
    }
    *string = 0;

    return start;
}

static struct CodeArray parse_expectations_from_file(struct apigen_MemoryArena * const arena, FILE * file)
{
    char line[1024]; // should be enough, really

    APIGEN_ASSERT(fseek(file, SEEK_SET, 0) == 0); // seek to start of file

    size_t line_length = fread(line, 1, sizeof(line) - 1, file);
    APIGEN_ASSERT(line_length <= sizeof(line));

    line[line_length] = 0;
    for (size_t i = 0; i < line_length; i++) {
        if (line[i] == '\r' || line[i] == '\n') {
            line[i]     = 0;
            line_length = i;
            break;
        }
    }

    static const size_t max_expected_items = 64;
    struct CodeArray    result             = {
                       .ptr = apigen_memory_arena_alloc(arena, max_expected_items * sizeof(enum apigen_DiagnosticCode)),
                       .len = 0,
    };

    if (apigen_starts_with(line, "// expected:")) {

        char * list = line + 12; // cut off the prefix

        while (*list) {
            size_t comma_pos;
            for (comma_pos = 0; list[comma_pos]; comma_pos++) {
                if (list[comma_pos] == ',') {
                    break;
                }
            }

            bool const end  = (list[comma_pos] == 0);
            list[comma_pos] = 0;

            char * item = str_trim(list);
            if (*item != 0) {
                long const expected_code = strtol(item, NULL, 10);
                if (expected_code != 0) {
                    if (result.len >= max_expected_items) {
                        apigen_panic("please adjust max_expected_items to successfully run this test!");
                    }
                    result.ptr[result.len] = (enum apigen_DiagnosticCode)expected_code;
                    result.len += 1;
                }
                else {
                    apigen_panic("invalid code specified in file. pleaes fix the test file!");
                }
            }

            if (end)
                break;
            list += (comma_pos + 1);
        }
    }

    (void)arena;

    APIGEN_ASSERT(fseek(file, SEEK_SET, 0) == 0); // revert to start of file

    return result;
}

int apigen_test_runner(
    struct apigen_MemoryArena * const arena,
    struct apigen_Diagnostics * const diagnostics,
    struct CliOptions const * const   options)
{
    if (options->positional_count != 1) {
        fprintf(stderr, "error: apigen requires exactly one positional file!\n");
        return EXIT_FAILURE;
    }

    FILE * f = fopen(options->positionals[0], "rb");
    if (f == NULL) {
        fprintf(stderr, "error: could not open %s!\n", options->positionals[0]);
        return 1;
    }

    struct CodeArray const expectations = parse_expectations_from_file(arena, f);
    
    fclose(f);

    struct apigen_ParserState state = {
        .source_dir  = apigen_io_cwd(),
        .file        = apigen_io_null,
        .file_name   = NULL,
        .ast_arena   = arena,
        .line_feed   = "\r\n",
        .diagnostics = diagnostics,
    };

    if(!apigen_open_input_from_cwd(&state, options->positionals[0])) {
        return EXIT_FAILURE;
    }

    bool ok = apigen_parse(&state);

    if (ok && (options->test_mode >= TEST_MODE_ANALYZER)) {
        struct apigen_Document document;
        ok = apigen_analyze(&state, &document);
    }

    if (expectations.len > 0) {
        bool expectations_met = true;

        for (size_t i = 0; i < expectations.len; i++) {
            enum apigen_DiagnosticCode const code = expectations.ptr[i];

            if (!apigen_diagnostics_remove_one(diagnostics, code)) {
                fprintf(stderr, "error: expected diagnostic code %d, but it was not present!\n", code);
                expectations_met = false;
            }
        }

        if (apigen_diagnostics_has_any(diagnostics)) {
            // got unexpected diagnostics, test failed
            fprintf(stderr, "error: unexpected diagnostics are present!\n");
            return false;
        }

        // all diagnostics are as expected, we successfully ran the test
        return expectations_met ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    else {
        return ok ? EXIT_SUCCESS : EXIT_FAILURE;
    }
}
