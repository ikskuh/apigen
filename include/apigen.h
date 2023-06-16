#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define APIGEN_STR(_X) #_X
#define APIGEN_SSTR(_X) APIGEN_STR(_X)

#define APIGEN_NORETURN __attribute__((noreturn))
#define APIGEN_UNREACHABLE() __builtin_unreachable()
#define APIGEN_NOT_NULL(_X)                                                    \
  do {                                                                         \
    if ((_X) == NULL) {                                                        \
      apigen_panic(__FILE__ ":" APIGEN_SSTR(__LINE__) ": Value " #_X " was NULL!");                                  \
    }                                                                          \
  } while (false)

#define APIGEN_ASSERT(_Cond)                                                   \
  do {                                                                         \
    if ((_Cond) == 0)                                                          \
      apigen_panic(__FILE__ ":" APIGEN_SSTR(__LINE__) ":Assertion failure: " #_Cond " did not assert!");          \
  } while (false)

typedef void *apigen_Stream;
typedef void (*apigen_StreamWriter)(apigen_Stream stream, char const *string,
                                    size_t length);

#define APIGEN_IO_WRITE_STR(_Stream, _Writer, _X)                              \
  _Writer(_Stream, _X, strlen(_X))

void APIGEN_NORETURN apigen_panic(char const *msg);

bool apigen_streq(char const * str1, char const * str2);

void apigen_io_writeStdOut(apigen_Stream null_stream, char const *string,
                           size_t length);
void apigen_io_writeStdErr(apigen_Stream null_stream, char const *string,
                           size_t length);
void apigen_io_writeFile(apigen_Stream file, char const *string, size_t length);


// generic values:

enum apigen_ValueType {
    apigen_value_null,
    apigen_value_sint,
    apigen_value_uint,
    apigen_value_str,
};

struct apigen_Value {
    enum apigen_ValueType type;
    union {
        uint64_t     value_uint; ///< active when type==apigen_value_uint
        int64_t      value_sint; ///< active when type==apigen_value_sint
        char const * value_str;  ///< active when type==apigen_value_str
    };
};

enum apigen_TypeId {
  apigen_typeid_void,
  apigen_typeid_anyopaque,
  apigen_typeid_opaque,

  apigen_typeid_bool,

  apigen_typeid_uchar, // unsigned char
  apigen_typeid_ichar, // signed char
  apigen_typeid_char,  // char

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
  apigen_typeid_ptr_to_one,                       // extra: apigen_Type
  apigen_typeid_ptr_to_many,                      // extra: apigen_Type
  apigen_typeid_ptr_to_sentinelled_many,          // extra: apigen_Type
  apigen_typeid_nullable_ptr_to_many,             // extra: apigen_Type
  apigen_typeid_nullable_ptr_to_one,              // extra: apigen_Type
  apigen_typeid_nullable_ptr_to_sentinelled_many, // extra: apigen_Type

  apigen_typeid_const_ptr_to_one,                       // extra: apigen_Type
  apigen_typeid_const_ptr_to_many,                      // extra: apigen_Type
  apigen_typeid_const_ptr_to_sentinelled_many,          // extra: apigen_Type
  apigen_typeid_nullable_const_ptr_to_many,             // extra: apigen_Type
  apigen_typeid_nullable_const_ptr_to_one,              // extra: apigen_Type
  apigen_typeid_nullable_const_ptr_to_sentinelled_many, // extra: apigen_Type

  // compound types:
  apigen_typeid_enum,     // extra: apigen_Enum
  apigen_typeid_struct,   // extra: apigen_UnionOrStruct
  apigen_typeid_union,    // extra: apigen_UnionOrStruct
  apigen_typeid_array,    // extra: apigen_Array
  apigen_typeid_function, // extra: apigen_Function

  APIGEN_TYPEID_LIMIT,
};

struct apigen_Type {
  enum apigen_TypeId id;
  void *extra;           ///< Points to extra information to interpret this type.
  char const * name;     ///< Non-NULL if the type has was explicitly declared with a name. Otherwise, this type is anonymous
};

struct apigen_Array {
    uint64_t             size;
    struct apigen_Type * underlying_type;
};

struct apigen_EnumItem {
    char const * documentation;
    char const * name;
    union {
        uint64_t uvalue; ///< Active for all enums based on an unsigned integer
        int64_t  ivalue; ///< Active for all enums based on an signed integer
    };
};

struct apigen_Enum {
    struct apigen_Type const * underlying_type;
    size_t                     item_count;
    struct apigen_EnumItem *   items;
};

/// Type for struct fields, union fields and paramteres.
/// They all share the same structure, so we can use the same type here.
struct apigen_NamedValue {
    char const *               documentation;
    char const *               name;
    struct apigen_Type const * type;
};

struct apigen_UnionOrStruct {
    size_t                     field_count;
    struct apigen_NamedValue * fields;
};

struct apigen_Function {
    struct apigen_Type const * return_type;

    size_t                     parameter_count;
    struct apigen_NamedValue * parameters;
};

// memory module:

void * apigen_alloc(size_t size);
void apigen_free(void * ptr);

struct apigen_MemoryArenaChunk;

struct apigen_MemoryArena {
  struct apigen_MemoryArenaChunk *first_chunk;
  struct apigen_MemoryArenaChunk *last_chunk;
  size_t chunk_size;
};

void apigen_memory_arena_init(struct apigen_MemoryArena *arena);
void apigen_memory_arena_deinit(struct apigen_MemoryArena *arena);

void *apigen_memory_arena_alloc(struct apigen_MemoryArena *arena, size_t size);
char *apigen_memory_arena_dupestr(struct apigen_MemoryArena *arena, char const * str);


// type management:

struct apigen_TypePoolNamedType;
struct apigen_TypePoolCache;

struct apigen_TypePool {
    struct apigen_MemoryArena *       arena;

    struct apigen_TypePoolNamedType * named_types;
    struct apigen_TypePoolCache *     cache;
};

/// Looks up a type by name, returns `NULL` if no type named `name` exists.
struct apigen_Type const * apigen_lookup_type(struct apigen_TypePool const * pool, char const * name);


bool apigen_register_type(struct apigen_TypePool * pool, struct apigen_Type const * type);

/// Takes `type` and returns a canonical version of it for which pointer equality is 
/// given. The returned value has same lifetime as the `pool` parameter.
struct apigen_Type const * apigen_intern_type(struct apigen_TypePool * pool, struct apigen_Type const * type);

// documents:

struct apigen_Global {
    char const *               documentation;
    char const *               name;
    struct apigen_Type const * type;
    bool                       is_const;
};

struct apigen_Constant {
    char const *               documentation;
    char const *               name;
    struct apigen_Type const * type;
    struct apigen_Value        value;
};

struct apigen_Document {
    struct apigen_TypePool       type_pool;

    size_t                       type_count;
    struct apigen_Type const * * types;

    size_t                       function_count;
    struct apigen_Type const * * functions; ///< As functions have no body, we can store them as named types as well.

    size_t                       variable_count;
    struct apigen_Global *       variables;

    size_t                       constant_count;
    struct apigen_Constant *     constants;
};

// parser and analysis:

struct apigen_ParserDeclaration;

struct apigen_ParserState {
  FILE *file;
  char const * file_name;
  struct apigen_MemoryArena *ast_arena;
  char const * line_feed; ///< used for multiline strings

  // output data:
  struct apigen_ParserDeclaration * top_level_declarations;
};

/// Parses `state->file` into an AST stored in
/// `state->top_level_declarations`.
bool apigen_parse(struct apigen_ParserState *state);

/// Analyzes `state->top_level_declarations` into concrete
/// types and declarations
bool apigen_analyze(struct apigen_ParserState *state, struct apigen_Document * out_document);



// predefined types:
extern struct apigen_Type const apigen_type_void;
extern struct apigen_Type const apigen_type_anyopaque;
extern struct apigen_Type const apigen_type_bool;
extern struct apigen_Type const apigen_type_uchar;
extern struct apigen_Type const apigen_type_ichar;
extern struct apigen_Type const apigen_type_char;
extern struct apigen_Type const apigen_type_u8;
extern struct apigen_Type const apigen_type_u16;
extern struct apigen_Type const apigen_type_u32;
extern struct apigen_Type const apigen_type_u64;
extern struct apigen_Type const apigen_type_usize;
extern struct apigen_Type const apigen_type_c_ushort;
extern struct apigen_Type const apigen_type_c_uint;
extern struct apigen_Type const apigen_type_c_ulong;
extern struct apigen_Type const apigen_type_c_ulonglong;
extern struct apigen_Type const apigen_type_i8;
extern struct apigen_Type const apigen_type_i16;
extern struct apigen_Type const apigen_type_i32;
extern struct apigen_Type const apigen_type_i64;
extern struct apigen_Type const apigen_type_isize;
extern struct apigen_Type const apigen_type_c_short;
extern struct apigen_Type const apigen_type_c_int;
extern struct apigen_Type const apigen_type_c_long;
extern struct apigen_Type const apigen_type_c_longlong;

