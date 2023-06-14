#include "apigen.h"

#include <string.h>

static char const *const well_defined_type_string[APIGEN_TYPEID_LIMIT] = {
    [apigen_typeid_void] = "void",
    [apigen_typeid_anyopaque] = "anyopaque",
    [apigen_typeid_bool] = "bool",
    [apigen_typeid_uchar] = "u8",
    [apigen_typeid_ichar] = "ichar",
    [apigen_typeid_char] = "u8",
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
};

static void render_type(struct apigen_Generator const *gen, struct apigen_Type const *type, apigen_Stream stream,
                        apigen_StreamWriter writer)
{
    (void)gen;
    APIGEN_NOT_NULL(type);
    APIGEN_NOT_NULL(writer);

    char const *const fixed_type_name = well_defined_type_string[type->id];
    if (fixed_type_name != NULL)
    {
        writer(stream, fixed_type_name, strlen(fixed_type_name));
        return;
    }

    switch (type->id)
    {
    case apigen_typeid_ptr_to_one:
        apigen_panic("rendering apigen_typeid_ptr_to_one not implemented yet!");
    case apigen_typeid_ptr_to_many:
        apigen_panic("rendering apigen_typeid_ptr_to_many not implemented yet!");
    case apigen_typeid_ptr_to_sentinelled_many:
        apigen_panic("rendering apigen_typeid_ptr_to_sentinelled_many not implemented yet!");
    case apigen_typeid_nullable_ptr_to_one:
        apigen_panic("rendering apigen_typeid_nullable_ptr_to_one not implemented yet!");
    case apigen_typeid_nullable_ptr_to_many:
        apigen_panic("rendering apigen_typeid_nullable_ptr_to_many not implemented yet!");
    case apigen_typeid_nullable_ptr_to_sentinelled_many:
        apigen_panic("rendering apigen_typeid_nullable_ptr_to_sentinelled_many not implemented yet!");

    case apigen_typeid_enum:
        apigen_panic("rendering apigen_typeid_enum not implemented yet");
    case apigen_typeid_struct:
        apigen_panic("rendering apigen_typeid_struct not implemented yet");
    case apigen_typeid_union:
        apigen_panic("rendering apigen_typeid_union not implemented yet");
    case apigen_typeid_array:
        apigen_panic("rendering apigen_typeid_array not implemented yet");

    default:
        APIGEN_UNREACHABLE();
    }
}

struct apigen_Generator apigen_gen_zig = {
    .render_type = render_type,
};
