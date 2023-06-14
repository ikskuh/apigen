#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define APIGEN_NORETURN __attribute__((noreturn))
#define APIGEN_UNREACHABLE() __builtin_unreachable()
#define APIGEN_NOT_NULL(_X)                                                                                            \
    do                                                                                                                 \
    {                                                                                                                  \
        if ((_X) == NULL)                                                                                              \
        {                                                                                                              \
            apigen_panic("Value " #_X " was NULL");                                                                    \
        }                                                                                                              \
    } while (false)

typedef void *apigen_Stream;
typedef void (*apigen_StreamWriter)(apigen_Stream stream, char const *string, size_t length);

#define APIGEN_IO_WRITE_STR(_Stream, _Writer, _X) _Writer(_Stream, _X, strlen(_X))

void APIGEN_NORETURN apigen_panic(char const *msg);

void apigen_io_writeStdOut(apigen_Stream null_stream, char const *string, size_t length);
void apigen_io_writeStdErr(apigen_Stream null_stream, char const *string, size_t length);
void apigen_io_writeFile(apigen_Stream file, char const *string, size_t length);

enum apigen_TypeId
{
    apigen_typeid_void,
    apigen_typeid_anyopaque,

    apigen_typeid_bool,

    apigen_typeid_uchar, // unsigned char
    apigen_typeid_ichar, // signed char
    apigen_typeid_xchar, // char

    // unsigned ints:
    apigen_typeid_u8,
    apigen_typeid_u16,
    apigen_typeid_u32,
    apigen_typeid_u64,
    apigen_typeid_usize,
    apigen_typeid_c_ushort,
    apigen_typeid_c_uint,
    apigen_typeid_c_ulong,
    apigen_typeid_c_ulonglong,

    // signed ints:
    apigen_typeid_i8,
    apigen_typeid_i16,
    apigen_typeid_i32,
    apigen_typeid_i64,
    apigen_typeid_isize,
    apigen_typeid_c_short,
    apigen_typeid_c_int,
    apigen_typeid_c_long,
    apigen_typeid_c_longlong,

    // pointers:
    apigen_typeid_ptr_to_one,
    apigen_typeid_ptr_to_many,
    apigen_typeid_ptr_to_sentinelled_many,
    apigen_typeid_nullable_ptr_to_one,
    apigen_typeid_nullable_ptr_to_many,
    apigen_typeid_nullable_ptr_to_sentinelled_many,

    // compound types:
    apigen_typeid_enum,
    apigen_typeid_struct,
    apigen_typeid_union,
    apigen_typeid_array,
    apigen_typeid_opaque,

    APIGEN_TYPEID_LIMIT,
};

struct apigen_Type
{
    enum apigen_TypeId id;
    void *extra;
};

void apigen_type_free(struct apigen_Type *type);

struct apigen_Generator
{
    void (*render_type)(struct apigen_Generator const *, struct apigen_Type const *, apigen_Stream,
                        apigen_StreamWriter);
};

extern struct apigen_Generator apigen_gen_c;
extern struct apigen_Generator apigen_gen_cpp;
extern struct apigen_Generator apigen_gen_zig;
extern struct apigen_Generator apigen_gen_rust;

void apigen_generator_renderType(struct apigen_Generator const *generator, struct apigen_Type const *type,
                                 apigen_Stream stream, apigen_StreamWriter writer);


                                 struct apigen_memory_arena 
                                 {
int dummy;
                                 }; 