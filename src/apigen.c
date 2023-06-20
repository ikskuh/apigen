#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "apigen.h"

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

    size_t positional_count;
    char const * const * positionals;

    bool help;
    enum TestMode test_mode;
    char const * output;
    bool implementation;
    enum TargetLanguage language;
};

static void print_help(char const * exe, FILE * out)
{
    static char const help_string[] = 
        "%s [-h] [-o <file>] [-l <lang>] <input file>" "\n"
        "" "\n"
        "apigen is a tool to generate bindings and implementations for APIs that cross ABI boundaries." "\n"
        "" "\n"
        "Options:" "\n"
        "   -h, --help             Shows this help text" "\n"
        "   -o, --output <path>    Instead of printing the output to stdout, will write the output to <path>." "\n"
        "   -l, --language <lang>  Generates code for the given language. Valid options are: [c], c++, zig, rust, go" "\n"
        "   -i, --implementation   Generates an implementation stub, not a binding." "\n"
        // "" "\n"
    ;
    if(exe == NULL) {
        exe = "apigen";
    }
    fprintf(out, help_string, exe);
}

static char const * short_option_map[128] = {
    ['h'] = "help",
    ['o'] = "output",
    ['l'] = "language",
    ['i'] = "implementation",
};

static void move_arg_to_end(int argc, char ** argv, int index)
{
    APIGEN_ASSERT(index < argc);
    if(index + 1 == argc) {
        return; // quick path
    }

    char * save = argv[index];
    for(int i = index + 1; i < argc; i++) {
        argv[i - 1] = argv[i];
    }
    argv[argc - 1] = save;
}

static void APIGEN_NORETURN parse_option_error(char const * option, char const * error)
{
    (void)option;
    fprintf(stderr, "error while parsing option '%s': %s\n", option, error);
    exit(1);
}   

enum OptionConsumption {
    IGNORE_VALUE,
    CONSUME_VALUE,
};

static enum OptionConsumption parse_option(struct CliOptions * out, char const * option, char const * value)
{
    // fprintf(stderr, "option='%s', value='%s'\n", option, value);
    if(apigen_streq(option, "help")) {
        out->help = true;
        return IGNORE_VALUE;
    }
    else if(apigen_streq(option, "implementation")) {
        out->implementation = true;
        return IGNORE_VALUE;
    }
    else if(apigen_streq(option, "test-mode")) {
        if(value == NULL) {
            parse_option_error(option, "expects output file name");
        }
        else if(apigen_streq(value, "disabled")) {
            out->test_mode = TEST_MODE_DISABLED;
        }
        else if(apigen_streq(value, "parser")) {
            out->test_mode = TEST_MODE_PARSER;
        }
        else if(apigen_streq(value, "analyzer")) {
            out->test_mode = TEST_MODE_ANALYZER;
        }
        else {
            parse_option_error(option, "illegal value");
        }
        return CONSUME_VALUE;
    }
    else if(apigen_streq(option, "output")) {
        if(value == NULL) {
            parse_option_error(option, "expects output file name");
        }
        out->output = value;
        return CONSUME_VALUE;
    }
    else if(apigen_streq(option, "language")) {
        if(value == NULL) {
            parse_option_error(option, "expects language identifier");
        }
        else if(apigen_streq(value, "c")) {
            out->language = LANG_C;
        }
        else if(apigen_streq(value, "c++")) {
            out->language = LANG_CPP;
        }
        else if(apigen_streq(value, "rust")) {
            out->language = LANG_RUST;
        }
        else if(apigen_streq(value, "go")) {
            out->language = LANG_GO;
        }
        else if(apigen_streq(value, "zig")) {
            out->language = LANG_ZIG;
        }
        else {
            parse_option_error(option, "unknown language");
        }
        return CONSUME_VALUE;
    }
    else {
        parse_option_error(option, "illegal option");
    }
}

static struct CliOptions parse_options_or_exit(int argc, char ** argv)
{
    if(argc == 0) exit(EXIT_FAILURE);

    struct CliOptions options = {
        .executable       = argv[0],
        .test_mode        = TEST_MODE_DISABLED,
        .positional_count = 0,
        .positionals      = NULL,
        .output = NULL,
        .help = false,
    };

    int index = 1;
    bool allow_options = true;
    while(index < argc)
    {
        char * const arg = argv[index];
        size_t const arg_len   = strlen(arg);
        

        if(allow_options && (arg_len == 2) && (arg[0] == '-') && (arg[1] == '-')) {
            // lone "--", everything after here is positional
            index += 1;
            allow_options = false;
            break;
        }
        else if(allow_options && (arg_len > 2) && (arg[0] == '-') && (arg[1] == '-')) {
            // long "--???" option
            char * const option = arg + 2; // cut off "--"
            index += 1;

            size_t i;
            for(i = 0; i < arg_len - 2; i++) {
                if(option[i] == '=') {
                    option[i] = 0;
                    break;
                }
            }

            bool const has_value = (i != (arg_len - 2));

            char const * value = ((index + 1) < argc) ? argv[index] : NULL;
            if(has_value) {
                value = option + i + 1;
            }

            // fprintf(stderr, "option='%s', value='%s'\n", option, value);

            if(parse_option(&options, option, value)) {
                if(!has_value) {
                    // we got the value from the next argv, not from 
                    // the current one, so skip the next one
                    index += 1;
                }
            }
        }
        else if(allow_options && (arg_len > 1) && (arg[0] == '-')) {
            // short "-?" option

            char * const opt_string = arg + 1; // cut off "--"
            index += 1;

            size_t i;
            for(i = 0; i < arg_len - 2; i++) {
                if(opt_string[i] == '=') {
                    opt_string[i] = 0;
                    break;
                }
            }

            bool const has_value = (i != (arg_len - 2));

            char const * value = ((index + 1) < argc) ? argv[index] : NULL;
            if(has_value) {
                value = opt_string + i + 1;
            }

            size_t const opt_len = strlen(opt_string);

            for(size_t j = 0; j < opt_len; j++)
            {
                char c = opt_string[j];

                if((c > 0) && ((size_t)c < sizeof(short_option_map))) {
                    char const * opt_expanded = short_option_map[(size_t)c];
                    if(opt_expanded != NULL) {
                        char const * opt_value = (i == (opt_len-1)) ? value : NULL; // only last option sees the value

                        if(parse_option(&options, opt_expanded, opt_value)) {
                            APIGEN_ASSERT(i == (opt_len-1));
                            if(!has_value) {
                                // we got the value from the next argv, not from 
                                // the current one, so skip the next one
                                index += 1;
                            }
                        }
                    }
                    else {
                        char buf[2] = { c, 0 };
                        parse_option_error(buf, "illegal option");
                    }
                }
                else {
                    char buf[2] = { c, 0 };
                    parse_option_error(buf, "illegal option");
                }
            }
        }
        else {
            // positional argument
            move_arg_to_end(argc, argv, index);
            options.positional_count += 1;
            index += 1;
        }
    }

    options.positionals = (char const * const *) (argv + argc - options.positional_count);

    return options;
}

static int regular_invocation(
        struct apigen_MemoryArena * const arena, 
        struct apigen_Diagnostics * const diagnostics, 
        struct CliOptions const * const options
    )
{
    struct apigen_ParserState state = {
        .file = stdin,
        .file_name = "stdin",
        .ast_arena = arena,
        .line_feed = "\r\n",
        .diagnostics = diagnostics,
    };

    if(options->positional_count != 1) {
        fprintf(stderr, "error: apigen requires exactly one positional file!\n");
        return EXIT_FAILURE;
    }

    if(apigen_streq(options->positionals[0], "-")) {
        state.file = stdin;
        state.file_name = "stdin";
    }
    else {
        state.file = fopen(options->positionals[0], "rb");
        state.file_name = options->positionals[0];
        if(state.file == NULL) {
            fprintf(stderr, "error: could not open %s!\n", state.file_name);
            return EXIT_FAILURE;
        }
    }

    bool ok = apigen_parse(&state);
    if(ok) {
        struct apigen_Document document;

        ok = apigen_analyze(&state, &document);
        if(ok) {

            FILE * output;
            if((options->output == NULL) || apigen_streq(options->output, "-")) {
                output = stdout;
            }
            else {
                output = fopen(options->output, "wb");
                if(output == NULL) {
                    fprintf(stderr, "error: could not open %s!\n", options->output);
                    return EXIT_FAILURE;
                }
            }

            struct apigen_Stream out_stream = apigen_io_file_stream(output);

            switch(options->language) {
                case LANG_C:    ok = apigen_render_c(out_stream, arena, diagnostics, &document);    break;
                case LANG_CPP:  ok = apigen_render_cpp(out_stream, arena, diagnostics, &document);  break;
                case LANG_ZIG:  ok = apigen_render_zig(out_stream, arena, diagnostics, &document);  break;
                case LANG_RUST: ok = apigen_render_rust(out_stream, arena, diagnostics, &document); break;
                case LANG_GO:   ok = apigen_render_go(out_stream, arena, diagnostics, &document);   break;
            }

            if(output != stdout) {
                fclose(output);
            }
        }
    }

    if(state.file != stdin) {
        fclose(state.file);
    }

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

struct CodeArray
{
    size_t len;
    enum apigen_DiagnosticCode * ptr;
};

static char * str_trim(char * string)
{
    APIGEN_NOT_NULL(string);

    while(*string && isspace(*string)) {
        string += 1;
    }
    
    char * start = string;
    while(*string && !isspace(*string)) {
        string += 1;
    }
    *string = 0;

    return start;
}

static struct CodeArray parse_expectations_from_file(struct apigen_MemoryArena * const arena, FILE * file)
{
    char line[1024]; // should be enough, really

    APIGEN_ASSERT(fseek(file, SEEK_SET, 0) == 0); // seek to start of file

    size_t line_length = fread(line, 1, sizeof(line)-1, file);
    APIGEN_ASSERT(line_length <= sizeof(line));

    line[line_length] = 0;
    for(size_t i = 0; i < line_length; i++)
    {
        if(line[i] == '\r' || line[i] == '\n') {
            line[i] = 0;
            line_length = i;
            break;
        }
    }

    static const size_t max_expected_items = 64;
    struct CodeArray result = {
        .ptr = apigen_memory_arena_alloc(arena, max_expected_items * sizeof(enum apigen_DiagnosticCode)),
        .len = 0,
    };

    if(apigen_starts_with(line, "// expected:")) {

        char * list = line + 12; // cut off the prefix

        while(*list) {
            size_t comma_pos;
            for(comma_pos = 0; list[comma_pos]; comma_pos++) {
                if(list[comma_pos] == ',') {
                    break;
                }
            }

            bool const end = (list[comma_pos] == 0);
            list[comma_pos] = 0;

            char * item = str_trim(list);
            if(*item != 0) {
                int expected_code = strtol(item, NULL, 10);
                if(expected_code != 0) {
                    if(result.len >= max_expected_items) {
                        apigen_panic("please adjust max_expected_items to successfully run this test!");
                    }
                    result.ptr[result.len] = (enum apigen_DiagnosticCode)expected_code;
                    result.len += 1;
                }
                else {
                    apigen_panic("invalid code specified in file. pleaes fix the test file!");
                }
            }

            if(end)
                break;
            list += (comma_pos + 1);
        }
        
    }

    (void)arena;

    APIGEN_ASSERT(fseek(file, SEEK_SET, 0) == 0); // revert to start of file

    return result;
}

static int test_runner(
        struct apigen_MemoryArena * const arena, 
        struct apigen_Diagnostics * const diagnostics, 
        struct CliOptions const * const options
    )
{
    if(options->positional_count != 1) {
        fprintf(stderr, "error: apigen requires exactly one positional file!\n");
        return EXIT_FAILURE;
    }

    FILE * f = fopen(options->positionals[0], "rb");
    if(f == NULL) {
        fprintf(stderr, "error: could not open %s!\n", options->positionals[0]);
        return 1;
    }

    struct CodeArray const expectations = parse_expectations_from_file(arena, f);
    

    struct apigen_ParserState state = {
        .file = f,
        .file_name = options->positionals[0],
        .ast_arena = arena,
        .line_feed = "\r\n",
        .diagnostics = diagnostics,
    };

    bool ok = apigen_parse(&state);

    fclose(f);

    if(ok && (options->test_mode >= TEST_MODE_ANALYZER)) {
        struct apigen_Document document;
        ok = apigen_analyze(&state, &document);
    }

    if(expectations.len > 0) {
        bool expectations_met = true;

        for(size_t i = 0; i < expectations.len; i++)
        {
            enum apigen_DiagnosticCode const code = expectations.ptr[i];
            
            if(!apigen_diagnostics_remove_one(diagnostics, code)) {
                fprintf(stderr, "error: expected diagnostic code %d, but it was not present!\n", code);
                expectations_met = false;
            }
        }
        
        if(apigen_diagnostics_has_any(diagnostics)) {
            // got unexpected diagnostics, test failed
            fprintf(stderr, "error: unexpected diagnostics are present!\n");
            return false;
        }
        
        // all diagnostics are as expected, we successfully ran the test
        return expectations_met ? EXIT_SUCCESS : EXIT_FAILURE;;
    }
    else {
        return ok ? EXIT_SUCCESS : EXIT_FAILURE;
    }
}

static int wrapped_main(
        struct apigen_MemoryArena * const arena, 
        struct apigen_Diagnostics * const diagnostics, 
        struct CliOptions const * const options
    )
{
    if(options->help) {
        print_help(options->executable, stdout);
        return EXIT_SUCCESS;
    }

    if(options->test_mode == TEST_MODE_DISABLED) {
        return regular_invocation(arena, diagnostics, options);
    }
    else {
        return test_runner(arena, diagnostics, options);
    }    
}

int main(int argc, char **argv)
{
    struct CliOptions options = parse_options_or_exit(argc, argv);

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

static char const * builtin_type_name[APIGEN_TYPEID_LIMIT] = {
    [apigen_typeid_void] = "void",
    [apigen_typeid_anyopaque] = "anyopaque",
    [apigen_typeid_opaque] = "opaque",
    [apigen_typeid_bool] = "bool",
    [apigen_typeid_uchar] = "uchar",
    [apigen_typeid_ichar] = "ichar",
    [apigen_typeid_char] = "char",
    [apigen_typeid_u8] = "u8",
    [apigen_typeid_u16] = "u16",
    [apigen_typeid_u32] = "u32",
    [apigen_typeid_u64] = "u64",
    [apigen_typeid_usize] = "usize",
    [apigen_typeid_c_ushort] = "c_ushort",
    [apigen_typeid_c_uint] = "c_uint",
    [apigen_typeid_c_ulong] = "c_ulong",
    [apigen_typeid_c_ulonglong] = "c_ulonglong",
    [apigen_typeid_i8] = "i8",
    [apigen_typeid_i16] = "i16",
    [apigen_typeid_i32] = "i32",
    [apigen_typeid_i64] = "i64",
    [apigen_typeid_isize] = "isize",
    [apigen_typeid_c_short] = "c_short",
    [apigen_typeid_c_int] = "c_int",
    [apigen_typeid_c_long] = "c_long",
    [apigen_typeid_c_longlong] = "c_longlong",
    [apigen_typeid_ptr_to_one] = "*T",
    [apigen_typeid_ptr_to_many] = "[*]T",
    [apigen_typeid_ptr_to_sentinelled_many] = "[*:N]T",
    [apigen_typeid_nullable_ptr_to_one] = "?*T",
    [apigen_typeid_nullable_ptr_to_many] = "?[*]T",
    [apigen_typeid_nullable_ptr_to_sentinelled_many] = "?[*:N]T",
    [apigen_typeid_const_ptr_to_one] = "*const T",
    [apigen_typeid_const_ptr_to_many] = "[*]const T",
    [apigen_typeid_const_ptr_to_sentinelled_many] = "[*:N]const T",
    [apigen_typeid_nullable_const_ptr_to_one] = "?*const T",
    [apigen_typeid_nullable_const_ptr_to_many] = "?[*]const T",
    [apigen_typeid_nullable_const_ptr_to_sentinelled_many] = "?[*:N]const T",
    [apigen_typeid_enum] = "enum{}",
    [apigen_typeid_struct] = "struct{}",
    [apigen_typeid_union] = "union{}",
    [apigen_typeid_array] = "[N]T",
    [apigen_typeid_function] = "fn(…)T",
};

char const * apigen_type_str(enum apigen_TypeId id) {
    return builtin_type_name[id];
}

bool apigen_value_eql(struct apigen_Value const * val1, struct apigen_Value const * val2)
{
    APIGEN_NOT_NULL(val1);
    APIGEN_NOT_NULL(val2);

    if(val1 == val2) {
        return true;
    }

    if(val1->type != val2->type) {
        return false;
    }

    switch(val1->type) {
        case apigen_value_null: return true;
        case apigen_value_sint: return (val1->value_sint == val2->value_sint);
        case apigen_value_uint: return (val1->value_uint == val2->value_uint);
        case apigen_value_str:  return apigen_streq(val1->value_str, val2->value_str);
    }
}
