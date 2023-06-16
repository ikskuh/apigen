#include "apigen.h"



struct apigen_Type const apigen_type_void        = { .id = apigen_typeid_void };
struct apigen_Type const apigen_type_anyopaque   = { .id = apigen_typeid_anyopaque };
struct apigen_Type const apigen_type_bool        = { .id = apigen_typeid_bool };
struct apigen_Type const apigen_type_uchar       = { .id = apigen_typeid_uchar };
struct apigen_Type const apigen_type_ichar       = { .id = apigen_typeid_ichar };
struct apigen_Type const apigen_type_char        = { .id = apigen_typeid_char };
struct apigen_Type const apigen_type_u8          = { .id = apigen_typeid_u8 };
struct apigen_Type const apigen_type_u16         = { .id = apigen_typeid_u16 };
struct apigen_Type const apigen_type_u32         = { .id = apigen_typeid_u32 };
struct apigen_Type const apigen_type_u64         = { .id = apigen_typeid_u64 };
struct apigen_Type const apigen_type_usize       = { .id = apigen_typeid_usize };
struct apigen_Type const apigen_type_c_ushort    = { .id = apigen_typeid_c_ushort };
struct apigen_Type const apigen_type_c_uint      = { .id = apigen_typeid_c_uint };
struct apigen_Type const apigen_type_c_ulong     = { .id = apigen_typeid_c_ulong };
struct apigen_Type const apigen_type_c_ulonglong = { .id = apigen_typeid_c_ulonglong };
struct apigen_Type const apigen_type_i8          = { .id = apigen_typeid_i8 };
struct apigen_Type const apigen_type_i16         = { .id = apigen_typeid_i16 };
struct apigen_Type const apigen_type_i32         = { .id = apigen_typeid_i32 };
struct apigen_Type const apigen_type_i64         = { .id = apigen_typeid_i64 };
struct apigen_Type const apigen_type_isize       = { .id = apigen_typeid_isize };
struct apigen_Type const apigen_type_c_short     = { .id = apigen_typeid_c_short };
struct apigen_Type const apigen_type_c_int       = { .id = apigen_typeid_c_int };
struct apigen_Type const apigen_type_c_long      = { .id = apigen_typeid_c_long };
struct apigen_Type const apigen_type_c_longlong  = { .id = apigen_typeid_c_longlong };


/// Pool nodes contain a name <-> value association, stored
/// in a linked list. 
struct apigen_TypePoolNamedType
{
    struct apigen_TypePoolNamedType * next;
    char const * name;
    struct apigen_Type const * type;
};

/// The cache contains a linked list of just 
/// "type values". This is used for deduplicating types.
struct apigen_TypePoolCache
{
    int dummy;
};

struct apigen_Type const * apigen_lookup_type(struct apigen_TypePool const * pool, char const * type_name)
{
    APIGEN_NOT_NULL(pool);
    APIGEN_NOT_NULL(type_name);

    // first, do some stupid name matching:
    if(apigen_streq(type_name, "void"))        return &apigen_type_void;
    if(apigen_streq(type_name, "anyopaque"))   return &apigen_type_anyopaque;
    if(apigen_streq(type_name, "bool"))        return &apigen_type_bool;
    if(apigen_streq(type_name, "uchar"))       return &apigen_type_uchar;
    if(apigen_streq(type_name, "ichar"))       return &apigen_type_ichar;
    if(apigen_streq(type_name, "char"))        return &apigen_type_char;
    if(apigen_streq(type_name, "u8"))          return &apigen_type_u8;
    if(apigen_streq(type_name, "u16"))         return &apigen_type_u16;
    if(apigen_streq(type_name, "u32"))         return &apigen_type_u32;
    if(apigen_streq(type_name, "u64"))         return &apigen_type_u64;
    if(apigen_streq(type_name, "usize"))       return &apigen_type_usize;
    if(apigen_streq(type_name, "c_ushort"))    return &apigen_type_c_ushort;
    if(apigen_streq(type_name, "c_uint"))      return &apigen_type_c_uint;
    if(apigen_streq(type_name, "c_ulong"))     return &apigen_type_c_ulong;
    if(apigen_streq(type_name, "c_ulonglong")) return &apigen_type_c_ulonglong;
    if(apigen_streq(type_name, "i8"))          return &apigen_type_i8;
    if(apigen_streq(type_name, "i16"))         return &apigen_type_i16;
    if(apigen_streq(type_name, "i32"))         return &apigen_type_i32;
    if(apigen_streq(type_name, "i64"))         return &apigen_type_i64;
    if(apigen_streq(type_name, "isize"))       return &apigen_type_isize;
    if(apigen_streq(type_name, "c_short"))     return &apigen_type_c_short;
    if(apigen_streq(type_name, "c_int"))       return &apigen_type_c_int;
    if(apigen_streq(type_name, "c_long"))      return &apigen_type_c_long;
    if(apigen_streq(type_name, "c_longlong"))  return &apigen_type_c_longlong;

    // search in well-known types:
    {
        struct apigen_TypePoolNamedType const * iter = pool->named_types;
        while(iter) {
            if(apigen_streq(iter->name, type_name)) {
                return iter->type;
            }
            iter = iter->next;
        }
    }    

    return NULL;
}

struct apigen_Type const * apigen_intern_type(struct apigen_TypePool * pool, struct apigen_Type const * type)
{
    APIGEN_NOT_NULL(pool);
    APIGEN_NOT_NULL(type);
    
    apigen_panic("not implemented yet!");
}

bool apigen_register_type(struct apigen_TypePool * pool, struct apigen_Type const * type)
{
    APIGEN_NOT_NULL(pool);
    APIGEN_NOT_NULL(type);
    APIGEN_ASSERT(type->name != NULL);

    if(apigen_lookup_type(pool, type->name) != NULL) {
        return NULL;
    }

    struct apigen_TypePoolNamedType * const node = apigen_memory_arena_alloc(pool->arena, sizeof(struct apigen_TypePoolNamedType));
    *node = (struct apigen_TypePoolNamedType) {
        .next = pool->named_types,
        .name = apigen_memory_arena_dupestr(pool->arena, type->name),
        .type = type,
    };
    pool->named_types = node;
    return node->type;
}

