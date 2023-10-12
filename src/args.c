#include "apigen.h"
#include "apigen-internals.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void apigen_print_help(char const * exe, FILE * out)
{
    static char const help_string[] =
        "%s [-h] [-o <file>] [-l <lang>] <input file>\n"
        "\n"
        "apigen is a tool to generate bindings and implementations for APIs that cross ABI boundaries.\n"
        "\n"
        "Options:\n"
        "   -h, --help             Shows this help text\n"
        "   -o, --output <path>    Instead of printing the output to stdout, will write the output to <path>.\n"
        "   -l, --language <lang>  Generates code for the given language. Valid options are: [c], c++, zig, rust, go\n"
        "   -i, --implementation   Generates an implementation stub, not a binding.\n"
        // "" "\n"
        ;
    if (exe == NULL) {
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
    APIGEN_NOT_NULL(argv);
    APIGEN_ASSERT(index < argc);
    if (index + 1 == argc) {
        return; // quick path
    }

    char * save = argv[index];
    for (int i = index + 1; i < argc; i++) {
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

enum OptionConsumption
{
    IGNORE_VALUE,
    CONSUME_VALUE,
};

static enum OptionConsumption parse_option(struct CliOptions * out, char const * option, char const * value)
{
    // fprintf(stderr, "option='%s', value='%s'\n", option, value);
    if (apigen_streq(option, "help")) {
        out->help = true;
        return IGNORE_VALUE;
    }
    else if (apigen_streq(option, "implementation")) {
        out->implementation = true;
        return IGNORE_VALUE;
    }
    else if (apigen_streq(option, "test-mode")) {
        if (value == NULL) {
            parse_option_error(option, "expects output file name");
        }
        else if (apigen_streq(value, "disabled")) {
            out->test_mode = TEST_MODE_DISABLED;
        }
        else if (apigen_streq(value, "parser")) {
            out->test_mode = TEST_MODE_PARSER;
        }
        else if (apigen_streq(value, "analyzer")) {
            out->test_mode = TEST_MODE_ANALYZER;
        }
        else {
            parse_option_error(option, "illegal value");
        }
        return CONSUME_VALUE;
    }
    else if (apigen_streq(option, "output")) {
        if (value == NULL) {
            parse_option_error(option, "expects output file name");
        }
        out->output = value;
        return CONSUME_VALUE;
    }
    else if (apigen_streq(option, "language")) {
        if (value == NULL) {
            parse_option_error(option, "expects language identifier");
        }
        else if (apigen_streq(value, "c")) {
            out->language = LANG_C;
        }
        else if (apigen_streq(value, "c++")) {
            out->language = LANG_CPP;
        }
        else if (apigen_streq(value, "rust")) {
            out->language = LANG_RUST;
        }
        else if (apigen_streq(value, "go")) {
            out->language = LANG_GO;
        }
        else if (apigen_streq(value, "zig")) {
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

struct CliOptions apigen_parse_options_or_exit(int argc, char ** argv)
{
    if (argc == 0) {
        exit(EXIT_FAILURE);
    }

    struct CliOptions options = {
        .executable       = argv[0],
        .test_mode        = TEST_MODE_DISABLED,
        .positional_count = 0,
        .positionals      = NULL,
        .output           = NULL,
        .help             = false,
    };

    int  index         = 1;
    bool allow_options = true;
    while (index < argc) {
        char * const arg     = argv[index];
        const size_t arg_len = strlen(arg);

        if (allow_options && (arg_len == 2) && (arg[0] == '-') && (arg[1] == '-')) {
            // lone "--", everything after here is positional
            index += 1;
            allow_options = false;
            break;
        }
        else if (allow_options && (arg_len > 2) && (arg[0] == '-') && (arg[1] == '-')) {
            // long "--???" option
            char * const option = arg + 2; // cut off "--"
            index += 1;

            size_t i;
            for (i = 0; i < arg_len - 2; i++) {
                if (option[i] == '=') {
                    option[i] = 0;
                    break;
                }
            }

            bool const has_value = (i != (arg_len - 2));

            char const * value = ((index + 1) < argc) ? argv[index] : NULL;
            if (has_value) {
                value = option + i + 1;
            }

            // fprintf(stderr, "option='%s', value='%s'\n", option, value);

            if (parse_option(&options, option, value)) {
                if (!has_value) {
                    // we got the value from the next argv, not from
                    // the current one, so skip the next one
                    index += 1;
                }
            }
        }
        else if (allow_options && (arg_len > 1) && (arg[0] == '-')) {
            // short "-?" option

            char * const opt_string = arg + 1; // cut off "--"
            index += 1;

            size_t i;
            for (i = 0; i < arg_len - 2; i++) {
                if (opt_string[i] == '=') {
                    opt_string[i] = 0;
                    break;
                }
            }

            bool const has_value = (i != (arg_len - 2));

            char const * value = ((index + 1) < argc) ? argv[index] : NULL;
            if (has_value) {
                value = opt_string + i + 1;
            }

            const size_t opt_len = strlen(opt_string);

            for (size_t j = 0; j < opt_len; j++) {
                char c = opt_string[j];

                if ((c > 0) && ((size_t)c < sizeof(short_option_map))) {
                    char const * opt_expanded = short_option_map[(size_t)c];
                    if (opt_expanded != NULL) {
                        char const * opt_value = (i == (opt_len - 1)) ? value : NULL; // only last option sees the value

                        if (parse_option(&options, opt_expanded, opt_value)) {
                            APIGEN_ASSERT(i == (opt_len - 1));
                            if (!has_value) {
                                // we got the value from the next argv, not from
                                // the current one, so skip the next one
                                index += 1;
                            }
                        }
                    }
                    else {
                        char buf[2] = {c, 0};
                        parse_option_error(buf, "illegal option");
                    }
                }
                else {
                    char buf[2] = {c, 0};
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

    options.positionals = (char const * const *)(argv + argc - options.positional_count);

    return options;
}

