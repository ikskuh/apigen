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
    else if(argc == 3 && apigen_streq(argv[1], "--analyzer-test")) {
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

        bool ok = apigen_parse(&state);

        fclose(f);

        if(ok) {
            struct apigen_Document document;
            ok = apigen_analyze(&state, &document);
        }

        apigen_memory_arena_deinit(&central_arena);
        return ok ? EXIT_SUCCESS : EXIT_FAILURE;
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
