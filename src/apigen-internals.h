#pragma once 

#include "apigen.h"


// This function is not part of the API, but is used internally to enable
// advanced debug diagnostics with the help from Zig.
void apigen_enable_debug_diagnostics(void);

// This function is not part of the API, but is used internally to enable
// advanced debug diagnostics with the help from Zig.
void apigen_dump_stack_trace(void);

extern void * (*apigen_memory_alloc_backend)(size_t);
extern void (*apigen_memory_free_backend)(void*);

enum TestMode
{
    TEST_MODE_DISABLED = 0,
    TEST_MODE_PARSER   = 1,
    TEST_MODE_ANALYZER = 2,
};

enum TargetLanguage
{
    LANG_C = 0,
    LANG_CPP,
    LANG_ZIG,
    LANG_RUST,
    LANG_GO
};

struct CliOptions
{
    char const * executable;

    size_t               positional_count;
    char const * const * positionals;

    bool                help;
    enum TestMode       test_mode;
    char const *        output;
    bool                implementation;
    enum TargetLanguage language;
};

struct CliOptions apigen_parse_options_or_exit(int argc, char ** argv);

bool apigen_open_input_from_cwd(struct apigen_ParserState * state, char const * path);

int apigen_test_runner(
    struct apigen_MemoryArena * const arena,
    struct apigen_Diagnostics * const diagnostics,
    struct CliOptions const * const   options
);

void apigen_print_help(char const * exe, FILE * out);

