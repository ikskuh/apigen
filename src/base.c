#include "apigen.h"
#include "apigen-internals.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Default implementation does nothing, we can override this from
// other object files.
void APIGEN_WEAK apigen_enable_debug_diagnostics(void)
{
    // No debug diagnostics
}

void APIGEN_WEAK apigen_dump_stack_trace(void)
{    
    // No debug diagnostics
}

void APIGEN_NORETURN APIGEN_WEAK apigen_panic(char const *msg)
{
    (void)fprintf(stderr, "\r\nAPIGEN PANIC: %s\r\n\r\n", msg);
    (void)fflush(stderr);

    apigen_dump_stack_trace();

#ifdef NDEBUG
    exit(1);
#else
    abort();
#endif
}

bool apigen_streq(char const * str1, char const * str2)
{
    APIGEN_NOT_NULL(str1);
    APIGEN_NOT_NULL(str2);

    return (0 == strcmp(str1, str2));
}

bool apigen_starts_with(char const * str1, char const * str2)
{
    APIGEN_NOT_NULL(str1);
    APIGEN_NOT_NULL(str2);
    size_t const str1_len = strlen(str1);
    size_t const str2_len = strlen(str2);
    if(str1_len < str2_len) {
        return false;
    }
    return (memcmp(str1, str2, str2_len) == 0);
}

uint64_t apigen_parse_uint(char const * str, uint8_t base)
{
    APIGEN_NOT_NULL(str);
    
    // fprintf(stderr, "parse '%s' to base %d\n", str, base);

    uint64_t val = 0;
    
    while(*str) {
        char c = *str;

        val *= base;
        if(c >= '0' && c <= '9') {
            val += (uint64_t)(c - '0');
        }
        else if(c >= 'a' && c <= 'z') {
            val += (uint64_t)(10 + c - 'F');
        }
        else if(c >= 'A' && c <= 'Z') {
            val += (uint64_t)(10 + c - 'A');
        }
        else {
            apigen_panic("invalid value for apigen_parse_uint");
        }

        str += 1;
    }

    return val;
}

int64_t apigen_parse_sint(char const * str, uint8_t base)
{
    APIGEN_NOT_NULL(str);
    uint64_t value = apigen_parse_uint(str, base);

    if(value >= 0x8000000000000000ULL) { // do the overflow dance!
        apigen_panic("integer out of range. handle better!");    
    }
    
    return -(int64_t)value;
}

static char const * builtin_type_name[APIGEN_TYPEID_LIMIT] = {
    [apigen_typeid_void]                                   = "void",
    [apigen_typeid_anyopaque]                              = "anyopaque",
    [apigen_typeid_opaque]                                 = "opaque",
    [apigen_typeid_bool]                                   = "bool",
    [apigen_typeid_uchar]                                  = "uchar",
    [apigen_typeid_ichar]                                  = "ichar",
    [apigen_typeid_char]                                   = "char",
    [apigen_typeid_u8]                                     = "u8",
    [apigen_typeid_u16]                                    = "u16",
    [apigen_typeid_u32]                                    = "u32",
    [apigen_typeid_u64]                                    = "u64",
    [apigen_typeid_usize]                                  = "usize",
    [apigen_typeid_c_ushort]                               = "c_ushort",
    [apigen_typeid_c_uint]                                 = "c_uint",
    [apigen_typeid_c_ulong]                                = "c_ulong",
    [apigen_typeid_c_ulonglong]                            = "c_ulonglong",
    [apigen_typeid_i8]                                     = "i8",
    [apigen_typeid_i16]                                    = "i16",
    [apigen_typeid_i32]                                    = "i32",
    [apigen_typeid_i64]                                    = "i64",
    [apigen_typeid_isize]                                  = "isize",
    [apigen_typeid_c_short]                                = "c_short",
    [apigen_typeid_c_int]                                  = "c_int",
    [apigen_typeid_c_long]                                 = "c_long",
    [apigen_typeid_c_longlong]                             = "c_longlong",
    [apigen_typeid_ptr_to_one]                             = "*T",
    [apigen_typeid_ptr_to_many]                            = "[*]T",
    [apigen_typeid_ptr_to_sentinelled_many]                = "[*:N]T",
    [apigen_typeid_nullable_ptr_to_one]                    = "?*T",
    [apigen_typeid_nullable_ptr_to_many]                   = "?[*]T",
    [apigen_typeid_nullable_ptr_to_sentinelled_many]       = "?[*:N]T",
    [apigen_typeid_const_ptr_to_one]                       = "*const T",
    [apigen_typeid_const_ptr_to_many]                      = "[*]const T",
    [apigen_typeid_const_ptr_to_sentinelled_many]          = "[*:N]const T",
    [apigen_typeid_nullable_const_ptr_to_one]              = "?*const T",
    [apigen_typeid_nullable_const_ptr_to_many]             = "?[*]const T",
    [apigen_typeid_nullable_const_ptr_to_sentinelled_many] = "?[*:N]const T",
    [apigen_typeid_enum]                                   = "enum{}",
    [apigen_typeid_struct]                                 = "struct{}",
    [apigen_typeid_union]                                  = "union{}",
    [apigen_typeid_array]                                  = "[N]T",
    [apigen_typeid_function]                               = "fn(…)T",
};

char const * apigen_type_str(enum apigen_TypeId id)
{
    return builtin_type_name[id];
}

bool apigen_value_eql(struct apigen_Value const * val1, struct apigen_Value const * val2)
{
    APIGEN_NOT_NULL(val1);
    APIGEN_NOT_NULL(val2);

    if (val1 == val2) {
        return true;
    }

    if (val1->type != val2->type) {
        return false;
    }

    switch (val1->type) {
        case apigen_value_null:
            return true;
        case apigen_value_sint:
            return (val1->value_sint == val2->value_sint);
        case apigen_value_uint:
            return (val1->value_uint == val2->value_uint);
        case apigen_value_str:
            return apigen_streq(val1->value_str, val2->value_str);
    }
}

bool apigen_open_input_from_cwd(struct apigen_ParserState * state, char const * path)
{
    if (apigen_streq(path, "-")) {
        state->file      = apigen_io_stdin;
        state->file_name = "stdin";
        return true;
    }
    
    state->file_name = path;
    if(!apigen_io_open_file_read(state->source_dir, path, &state->file)) {
        fprintf(stderr, "error: could not open %s!\n", state->file_name);
        return false;
    }

    char * const dirname = apigen_io_dirname(path);
    if(dirname != NULL) {
        if(!apigen_io_open_dir(state->source_dir, dirname, &state->source_dir)) {
            return false;
        }
        apigen_free(dirname);
    }

    return true;
}

