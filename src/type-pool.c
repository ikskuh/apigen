#include "apigen.h"

#include <string.h>

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
struct apigen_Type const apigen_type_f32         = { .id = apigen_typeid_f32 };
struct apigen_Type const apigen_type_f64         = { .id = apigen_typeid_f64 };


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
    struct apigen_TypePoolCache * next;

    struct apigen_Type interned_type;
};

struct apigen_Type const * apigen_lookup_type(struct apigen_TypePool const * pool, char const * type_name)
{
    APIGEN_NOT_NULL(pool);
    APIGEN_NOT_NULL(type_name);

    // first, do some stupid name matching:
    if(apigen_streq(type_name, "void"))        return &apigen_type_void;
    if(apigen_streq(type_name, "anyopaque"))   return &apigen_type_anyopaque;
    if(apigen_streq(type_name, "bool"))        return &apigen_type_bool;
    if(apigen_streq(type_name, "c_uchar"))     return &apigen_type_uchar;
    if(apigen_streq(type_name, "c_ichar"))     return &apigen_type_ichar;
    if(apigen_streq(type_name, "c_char"))      return &apigen_type_char;
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
    if(apigen_streq(type_name, "f32"))         return &apigen_type_f32;
    if(apigen_streq(type_name, "f64"))         return &apigen_type_f64;

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

bool apigen_register_type(struct apigen_TypePool * pool, struct apigen_Type const * type, char const * name_hint)
{
    APIGEN_NOT_NULL(pool);
    APIGEN_NOT_NULL(type);

    char const * const type_name = (name_hint != NULL) ? name_hint : type->name;
    
    APIGEN_ASSERT(type_name != NULL);

    if(apigen_lookup_type(pool, type_name) != NULL) {
        return NULL;
    }

    struct apigen_TypePoolNamedType * const node = apigen_memory_arena_alloc(pool->arena, sizeof(struct apigen_TypePoolNamedType));
    *node = (struct apigen_TypePoolNamedType) {
        .next = pool->named_types,
        .name = apigen_memory_arena_dupestr(pool->arena, type_name),
        .type = type,
    };
    pool->named_types = node;
    return node->type;
}



static bool apigen_is_type_unique(enum apigen_TypeId id) {
    switch(id) {
        case apigen_typeid_opaque: return true;
        case apigen_typeid_enum:   return true;
        case apigen_typeid_struct: return true;
        case apigen_typeid_union:  return true;
        default:                   return false;
    }
}

static bool apigen_is_type_builtin(enum apigen_TypeId id)
{
    switch(id) {
        case apigen_typeid_void:        return true;
        case apigen_typeid_anyopaque:   return true;
        case apigen_typeid_bool:        return true;
        case apigen_typeid_uchar:       return true;
        case apigen_typeid_ichar:       return true;
        case apigen_typeid_char:        return true;
        case apigen_typeid_u8:          return true;
        case apigen_typeid_u16:         return true;
        case apigen_typeid_u32:         return true;
        case apigen_typeid_u64:         return true;
        case apigen_typeid_usize:       return true;
        case apigen_typeid_c_ushort:    return true;
        case apigen_typeid_c_uint:      return true;
        case apigen_typeid_c_ulong:     return true;
        case apigen_typeid_c_ulonglong: return true;
        case apigen_typeid_i8:          return true;
        case apigen_typeid_i16:         return true;
        case apigen_typeid_i32:         return true;
        case apigen_typeid_i64:         return true;
        case apigen_typeid_isize:       return true;
        case apigen_typeid_c_short:     return true;
        case apigen_typeid_c_int:       return true;
        case apigen_typeid_c_long:      return true;
        case apigen_typeid_c_longlong:  return true;
        case apigen_typeid_f32:  return true;
        case apigen_typeid_f64:  return true;
        default:                        return false;
    }
}

struct apigen_Type const * apigen_get_builtin_type(enum apigen_TypeId id)
{
    switch(id) {
        case apigen_typeid_void:        return &apigen_type_void;
        case apigen_typeid_anyopaque:   return &apigen_type_anyopaque;
        case apigen_typeid_bool:        return &apigen_type_bool;
        case apigen_typeid_uchar:       return &apigen_type_uchar;
        case apigen_typeid_ichar:       return &apigen_type_ichar;
        case apigen_typeid_char:        return &apigen_type_char;
        case apigen_typeid_u8:          return &apigen_type_u8;
        case apigen_typeid_u16:         return &apigen_type_u16;
        case apigen_typeid_u32:         return &apigen_type_u32;
        case apigen_typeid_u64:         return &apigen_type_u64;
        case apigen_typeid_usize:       return &apigen_type_usize;
        case apigen_typeid_c_ushort:    return &apigen_type_c_ushort;
        case apigen_typeid_c_uint:      return &apigen_type_c_uint;
        case apigen_typeid_c_ulong:     return &apigen_type_c_ulong;
        case apigen_typeid_c_ulonglong: return &apigen_type_c_ulonglong;
        case apigen_typeid_i8:          return &apigen_type_i8;
        case apigen_typeid_i16:         return &apigen_type_i16;
        case apigen_typeid_i32:         return &apigen_type_i32;
        case apigen_typeid_i64:         return &apigen_type_i64;
        case apigen_typeid_isize:       return &apigen_type_isize;
        case apigen_typeid_c_short:     return &apigen_type_c_short;
        case apigen_typeid_c_int:       return &apigen_type_c_int;
        case apigen_typeid_c_long:      return &apigen_type_c_long;
        case apigen_typeid_c_longlong:  return &apigen_type_c_longlong;
        case apigen_typeid_f32:         return &apigen_type_f32;
        case apigen_typeid_f64:         return &apigen_type_f64;
        default:                        return NULL;
    }
}

static bool is_sentinelled_ptr(enum apigen_TypeId id)
{
    if(id == apigen_typeid_ptr_to_sentinelled_many) return true;
    if(id == apigen_typeid_nullable_ptr_to_sentinelled_many) return true;
    if(id == apigen_typeid_const_ptr_to_sentinelled_many) return true;
    if(id == apigen_typeid_nullable_const_ptr_to_sentinelled_many) return true;

    return false;
}

bool apigen_type_eql(struct apigen_Type const * type1, struct apigen_Type const * type2)
{
    APIGEN_NOT_NULL(type1);
    APIGEN_NOT_NULL(type2);
    
    if(type1 == type2) {
        return true;
    }
    
    if(type1->id != type2->id) {
        return false;
    }

    APIGEN_ASSERT(!apigen_is_type_builtin(type1->id)); // Builtins must be pointer-equal
    
    if(apigen_is_type_unique(type1->id)) {
        // Unique types are pointer-equal, no deep equality
        return false;
    }

    switch(type1->id)
    {
        case apigen_typeid_alias:       __builtin_unreachable();
        
        case apigen_typeid_void:        __builtin_unreachable();
        case apigen_typeid_anyopaque:   __builtin_unreachable();
        case apigen_typeid_bool:        __builtin_unreachable();
        case apigen_typeid_uchar:       __builtin_unreachable();
        case apigen_typeid_ichar:       __builtin_unreachable();
        case apigen_typeid_char:        __builtin_unreachable();
        case apigen_typeid_u8:          __builtin_unreachable();
        case apigen_typeid_u16:         __builtin_unreachable();
        case apigen_typeid_u32:         __builtin_unreachable();
        case apigen_typeid_u64:         __builtin_unreachable();
        case apigen_typeid_usize:       __builtin_unreachable();
        case apigen_typeid_c_ushort:    __builtin_unreachable();
        case apigen_typeid_c_uint:      __builtin_unreachable();
        case apigen_typeid_c_ulong:     __builtin_unreachable();
        case apigen_typeid_c_ulonglong: __builtin_unreachable();
        case apigen_typeid_i8:          __builtin_unreachable();
        case apigen_typeid_i16:         __builtin_unreachable();
        case apigen_typeid_i32:         __builtin_unreachable();
        case apigen_typeid_i64:         __builtin_unreachable();
        case apigen_typeid_isize:       __builtin_unreachable();
        case apigen_typeid_c_short:     __builtin_unreachable();
        case apigen_typeid_c_int:       __builtin_unreachable();
        case apigen_typeid_c_long:      __builtin_unreachable();
        case apigen_typeid_c_longlong:  __builtin_unreachable();
        case apigen_typeid_f32:         __builtin_unreachable();
        case apigen_typeid_f64:         __builtin_unreachable();

        case apigen_typeid_opaque:      __builtin_unreachable();
        case apigen_typeid_enum:        __builtin_unreachable();
        case apigen_typeid_struct:      __builtin_unreachable();
        case apigen_typeid_union:       __builtin_unreachable();

        case apigen_typeid_ptr_to_one:
        case apigen_typeid_ptr_to_many:
        case apigen_typeid_ptr_to_sentinelled_many:
        case apigen_typeid_nullable_ptr_to_one:
        case apigen_typeid_nullable_ptr_to_many:
        case apigen_typeid_nullable_ptr_to_sentinelled_many:
        case apigen_typeid_const_ptr_to_one:
        case apigen_typeid_const_ptr_to_many:
        case apigen_typeid_const_ptr_to_sentinelled_many:
        case apigen_typeid_nullable_const_ptr_to_one:
        case apigen_typeid_nullable_const_ptr_to_many:
        case apigen_typeid_nullable_const_ptr_to_sentinelled_many: {
            APIGEN_NOT_NULL(type1->extra);
            APIGEN_NOT_NULL(type2->extra);

            struct apigen_Pointer const * const extra1 = type1->extra;
            struct apigen_Pointer const * const extra2 = type2->extra;

            if(!apigen_type_eql(extra1->underlying_type, extra2->underlying_type)) {
                return false;
            }

            if(is_sentinelled_ptr(type1->id) && !apigen_value_eql(&extra1->sentinel, &extra2->sentinel)) {
                return false;
            }

            return true;
        }   

        case apigen_typeid_array: {
            APIGEN_NOT_NULL(type1->extra);
            APIGEN_NOT_NULL(type2->extra);

            struct apigen_Array const * const extra1 = type1->extra;
            struct apigen_Array const * const extra2 = type2->extra;
            
            if(!apigen_type_eql(extra1->underlying_type, extra2->underlying_type)) {
                return false;
            }

            if(extra1->size != extra2->size) {
                return false;
            }

            return true;
        }

        case apigen_typeid_function: {
            APIGEN_NOT_NULL(type1->extra);
            APIGEN_NOT_NULL(type2->extra);

            struct apigen_FunctionType const * const extra1 = type1->extra;
            struct apigen_FunctionType const * const extra2 = type2->extra;
            
            if(extra1->parameter_count != extra2->parameter_count) {
                return false;
            }

            if(!apigen_type_eql(extra1->return_type, extra2->return_type)) {
                return false;
            }

            for(size_t i = 0; i < extra1->parameter_count; i++)
            {
                if(!apigen_streq(extra1->parameters[i].name, extra2->parameters[i].name)) {
                    return false;
                }

                bool const doc1 = (extra1->parameters[i].documentation != NULL);
                bool const doc2 = (extra2->parameters[i].documentation != NULL);

                if(doc1 != doc2) {
                    return false;
                }
                
                if(doc1 && !apigen_streq(extra1->parameters[i].documentation, extra2->parameters[i].documentation)) {
                    return false;
                }
                
                if(!apigen_type_eql(extra1->parameters[i].type, extra2->parameters[i].type)) {
                    return false;
                }
            }

            return true;
        }


        case APIGEN_TYPEID_LIMIT:       __builtin_unreachable();
    }    
}

struct apigen_Type const * apigen_intern_type(struct apigen_TypePool * pool, struct apigen_Type const * unchecked_type)
{
    APIGEN_NOT_NULL(pool);
    APIGEN_NOT_NULL(unchecked_type);

    if(apigen_is_type_unique(unchecked_type->id)) {
        // unique types are unique, they cannot be interned
        return unchecked_type;
    }
    
    if(apigen_is_type_builtin(unchecked_type->id)) {
        // builtin types are assumed to be static, no need for interning
        APIGEN_ASSERT(apigen_get_builtin_type(unchecked_type->id) == unchecked_type);
        return unchecked_type;
    }

    struct apigen_TypePoolCache * cache_entry = pool->cache;
    while(cache_entry) {
        if(apigen_type_eql(&cache_entry->interned_type, unchecked_type)) {
            // fprintf(stderr, "cache hit for %s\n", apigen_type_str(unchecked_type->id));
            return &cache_entry->interned_type;
        }
        cache_entry = cache_entry->next;
    }

    // TYPE was not inserted into the intern pool yet
    cache_entry = apigen_memory_arena_alloc(pool->arena, sizeof(struct apigen_TypePoolCache));
    *cache_entry = (struct apigen_TypePoolCache) {
        .interned_type = *unchecked_type,
        .next = pool->cache,        
    };

    // duplicate extra-storage
    size_t extra_size = 0;
    switch(cache_entry->interned_type.id) {
        case apigen_typeid_ptr_to_one:
        case apigen_typeid_ptr_to_many:
        case apigen_typeid_ptr_to_sentinelled_many:
        case apigen_typeid_nullable_ptr_to_one:
        case apigen_typeid_nullable_ptr_to_many:
        case apigen_typeid_nullable_ptr_to_sentinelled_many:
        case apigen_typeid_const_ptr_to_one:
        case apigen_typeid_const_ptr_to_many:
        case apigen_typeid_const_ptr_to_sentinelled_many:
        case apigen_typeid_nullable_const_ptr_to_one:
        case apigen_typeid_nullable_const_ptr_to_many:
        case apigen_typeid_nullable_const_ptr_to_sentinelled_many:
            extra_size = sizeof(struct apigen_Pointer);
            break;
        case apigen_typeid_enum:
            extra_size = sizeof(struct apigen_Enum);
            break;

        case apigen_typeid_struct:
            extra_size = sizeof(struct apigen_UnionOrStruct);
            break;

        case apigen_typeid_union:
            extra_size = sizeof(struct apigen_UnionOrStruct);
            break;

        case apigen_typeid_array:
            extra_size = sizeof(struct apigen_Array);
            break;

        case apigen_typeid_function:
            extra_size = sizeof(struct apigen_FunctionType);
            break;
         
        default:
            extra_size = 0;
            break;
    }

    if(extra_size > 0) {
        void * extra_storage = apigen_memory_arena_alloc(pool->arena, extra_size);
        memcpy( extra_storage, unchecked_type->extra, extra_size);
        cache_entry->interned_type.extra = extra_storage;
    }

    // fprintf(stderr, "cache insert for %s\n", apigen_type_str(unchecked_type->id));

    pool->cache = cache_entry;

    return &cache_entry->interned_type;
}


bool apigen_type_is_primitive_type(enum apigen_TypeId type)
{
    switch(type) {
        case apigen_typeid_void:                                   return true;
        case apigen_typeid_anyopaque:                              return true;
        case apigen_typeid_bool:                                   return true;
        case apigen_typeid_uchar:                                  return true;
        case apigen_typeid_ichar:                                  return true;
        case apigen_typeid_char:                                   return true;

        case apigen_typeid_u8:                                     return true;
        case apigen_typeid_u16:                                    return true;
        case apigen_typeid_u32:                                    return true;
        case apigen_typeid_u64:                                    return true;
        case apigen_typeid_usize:                                  return true;
        case apigen_typeid_c_ushort:                               return true;
        case apigen_typeid_c_uint:                                 return true;
        case apigen_typeid_c_ulong:                                return true;
        case apigen_typeid_c_ulonglong:                            return true;

        case apigen_typeid_i8:                                     return true;
        case apigen_typeid_i16:                                    return true;
        case apigen_typeid_i32:                                    return true;
        case apigen_typeid_i64:                                    return true;
        case apigen_typeid_isize:                                  return true;
        case apigen_typeid_c_short:                                return true;
        case apigen_typeid_c_int:                                  return true;
        case apigen_typeid_c_long:                                 return true;
        case apigen_typeid_c_longlong:                             return true;

        case apigen_typeid_f32:                                    return true;
        case apigen_typeid_f64:                                    return true;

        case apigen_typeid_ptr_to_one:                             return true;
        case apigen_typeid_ptr_to_many:                            return true;
        case apigen_typeid_ptr_to_sentinelled_many:                return true;
        case apigen_typeid_nullable_ptr_to_one:                    return true;
        case apigen_typeid_nullable_ptr_to_many:                   return true;
        case apigen_typeid_nullable_ptr_to_sentinelled_many:       return true;
        case apigen_typeid_const_ptr_to_one:                       return true;
        case apigen_typeid_const_ptr_to_many:                      return true;
        case apigen_typeid_const_ptr_to_sentinelled_many:          return true;
        case apigen_typeid_nullable_const_ptr_to_one:              return true;
        case apigen_typeid_nullable_const_ptr_to_many:             return true;
        case apigen_typeid_nullable_const_ptr_to_sentinelled_many: return true;
        
        case apigen_typeid_array:                                  return true;

        case apigen_typeid_opaque:                                 return false;
        case apigen_typeid_enum:                                   return false;
        case apigen_typeid_struct:                                 return false;
        case apigen_typeid_union:                                  return false;
        case apigen_typeid_function:                               return false;

        case apigen_typeid_alias:                                  return false;

        case APIGEN_TYPEID_LIMIT:                                  APIGEN_UNREACHABLE();
    }
}