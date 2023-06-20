#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

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

struct apigen_Stream 
{
    void * context;
    void (*write)(void * context, char const * data, size_t length);
};

void APIGEN_NORETURN apigen_panic(char const *msg);

bool apigen_starts_with(char const * str1, char const * str2);
bool apigen_streq(char const * str1, char const * str2);

extern struct apigen_Stream const apigen_io_stdout;
extern struct apigen_Stream const apigen_io_stderr;
extern struct apigen_Stream const apigen_io_null;

struct apigen_Stream apigen_io_file_stream(FILE * file);

void apigen_io_write(struct apigen_Stream stream, char const * data, size_t length);
void apigen_io_printf(struct apigen_Stream stream, char const * format, ...);
void apigen_io_vprintf(struct apigen_Stream stream, char const * format, va_list list);
void apigen_io_write_string(struct apigen_Stream stream, char const * data);

// generic values:

enum apigen_ValueType
{
    apigen_value_null,
    apigen_value_sint,
    apigen_value_uint,
    apigen_value_str,
};

struct apigen_Value
{
    enum apigen_ValueType type;
    union {
        uint64_t     value_uint; ///< active when type==apigen_value_uint
        int64_t      value_sint; ///< active when type==apigen_value_sint
        char const * value_str;  ///< active when type==apigen_value_str
    };
};

uint64_t apigen_parse_uint(char const * str, uint8_t base); ///< parses a positive integer, no sign contained in `str`
int64_t  apigen_parse_sint(char const * str, uint8_t base); ///< parses a negative integer, no sign contained in `str`

bool apigen_value_eql(struct apigen_Value const * val1, struct apigen_Value const * val2);

enum apigen_TypeId {
  apigen_typeid_void,
  apigen_typeid_anyopaque,
  apigen_typeid_opaque, // extra: none

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

  // floats
  apigen_typeid_f32,
  apigen_typeid_f64,

  // pointers:
  apigen_typeid_ptr_to_one,                       // extra: apigen_Type
  apigen_typeid_ptr_to_many,                      // extra: apigen_Type
  apigen_typeid_ptr_to_sentinelled_many,          // extra: apigen_Type
  apigen_typeid_nullable_ptr_to_one,              // extra: apigen_Type
  apigen_typeid_nullable_ptr_to_many,             // extra: apigen_Type
  apigen_typeid_nullable_ptr_to_sentinelled_many, // extra: apigen_Pointer

  apigen_typeid_const_ptr_to_one,                       // extra: apigen_Pointer
  apigen_typeid_const_ptr_to_many,                      // extra: apigen_Pointer
  apigen_typeid_const_ptr_to_sentinelled_many,          // extra: apigen_Pointer
  apigen_typeid_nullable_const_ptr_to_one,              // extra: apigen_Pointer
  apigen_typeid_nullable_const_ptr_to_many,             // extra: apigen_Pointer
  apigen_typeid_nullable_const_ptr_to_sentinelled_many, // extra: apigen_Pointer

  // compound types:
  apigen_typeid_enum,     // extra: apigen_Enum
  apigen_typeid_struct,   // extra: apigen_UnionOrStruct
  apigen_typeid_union,    // extra: apigen_UnionOrStruct
  apigen_typeid_array,    // extra: apigen_Array
  apigen_typeid_function, // extra: apigen_FunctionType

  APIGEN_TYPEID_LIMIT,
};

struct apigen_Type {
  enum apigen_TypeId id;
  void const *       extra; ///< Points to extra information to interpret this type.
  char const *       name;  ///< Non-NULL if the type has was explicitly declared with a name. Otherwise, this type has no name assigned
  bool               is_anonymous; ///< Type was created implicitly in a nested type construct, but still requires a name.
};

struct apigen_Pointer {
    struct apigen_Type const * underlying_type;
    struct apigen_Value        sentinel;
};

struct apigen_Array {
    uint64_t                   size;
    struct apigen_Type const * underlying_type;
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

struct apigen_FunctionType {
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


bool apigen_register_type(struct apigen_TypePool * pool, struct apigen_Type const * type, char const * name_hint);

/// Takes `type` and returns a canonical version of it for which pointer equality is 
/// given. The returned value has same lifetime as the `pool` parameter.
struct apigen_Type const * apigen_intern_type(struct apigen_TypePool * pool, struct apigen_Type const * type);

char const * apigen_type_str(enum apigen_TypeId id);

bool apigen_type_eql(struct apigen_Type const * type1, struct apigen_Type const * type2);

struct apigen_Type const * apigen_get_builtin_type(enum apigen_TypeId id);

// documents:

struct apigen_Global {
    char const *               documentation;
    char const *               name;
    struct apigen_Type const * type;
    bool                       is_const;
};

struct apigen_Function {
    char const *               documentation;
    char const *               name;
    struct apigen_Type const * type;
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
    struct apigen_Function *     functions; ///< As functions have no body, we can store them as named types as well.

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
  struct apigen_Diagnostics * diagnostics;

  // output data:
  struct apigen_ParserDeclaration * top_level_declarations;
};

/// Parses `state->file` into an AST stored in
/// `state->top_level_declarations`.
bool apigen_parse(struct apigen_ParserState *state);

/// Analyzes `state->top_level_declarations` into concrete
/// types and declarations
bool apigen_analyze(struct apigen_ParserState *state, struct apigen_Document * out_document);

// diagnostics:

#define APIGEN_DIAGNOSTIC_FIRST_ERR   1000
#define APIGEN_DIAGNOSTIC_LAST_ERR    5999

#define APIGEN_DIAGNOSTIC_FIRST_WARN  6000
#define APIGEN_DIAGNOSTIC_LAST_WARN  11999

#define APIGEN_DIAGNOSTIC_FIRST_NOTE 12000
#define APIGEN_DIAGNOSTIC_LAST_NOTE  14999

// Expects _Mac(Symbol, ID, Format String)
#define APIGEN_EXPAND_DIAGNOSTIC_CODE_SET(_Mac)          \
    _Mac(apigen_error_array_size_not_uint,     1000, "Array size is not an unsigend integer")    \
    _Mac(apigen_error_duplicate_field,         1001, "Struct or enumeration already contains a member with the name '%s'")    \
    _Mac(apigen_error_duplicate_parameter,     1002, "A parameter with the name '%s' already exists")    \
    _Mac(apigen_error_duplicate_enum_item,     1003, "An enumeration member with the name 's' already exists")    \
    _Mac(apigen_error_duplicate_enum_value,    1004, "Enumeration member '%s' has value %s, which is already assigned to enumeration member '%s'")    \
    _Mac(apigen_error_enum_out_of_range,       1005, "Value %s is out of range for enumeration member '%s'")    \
    _Mac(apigen_error_enum_value_illegal,      1006, "Enumeration member '%s' has a non-integer value")    \
    _Mac(apigen_error_duplicate_symbol,        1007, "The symbol '%s' is already declared") \
    _Mac(apigen_error_syntax_error,            1008, "Syntax error at symbol '%s': %s")     \
    _Mac(apigen_error_undeclared_identifier,   1009, "Undeclared identifier '%s'")        \
    _Mac(apigen_error_unresolved_symbols,      1010, "Found %zu cyclic dependencies or undeclared types")  \
    _Mac(apigen_error_enum_type_must_be_int,   1011, "Enum backing type must be an integer") \
    _Mac(apigen_error_enum_empty,              1012, "Enums must contain at least one item") \
    _Mac(apigen_error_constexpr_type_mismatch, 1013, "The value assigned to constant '%s' does not match its type") \
    _Mac(apigen_error_constexpr_out_of_range,  1014, "The value assigned to constant '%s' is out of range") \
    _Mac(apigen_error_internal,                5999, "Internal compiler error") \
    \
    _Mac(apigen_warning_enum_int_undefined,    6000, "Chosen enum backing type %s has no well-defined range. Generated code may not be portable") \
    _Mac(apigen_warning_struct_empty,          6001, "An empty struct or union can be defined, but is not guaranteed to be portable between platforms or compilers.")

enum apigen_DiagnosticCode
{
#define APIGEN_TEMP_MACRO(_Symbol, _Id, _Format) _Symbol = _Id,
APIGEN_EXPAND_DIAGNOSTIC_CODE_SET(APIGEN_TEMP_MACRO)
#undef  APIGEN_TEMP_MACRO
};

struct apigen_DiagnosticItem;

#define APIGEN_DIAGNOSTIC_FLAG_ERROR (1U<<0)
#define APIGEN_DIAGNOSTIC_FLAG_WARN  (1U<<1)
#define APIGEN_DIAGNOSTIC_FLAG_NOTE  (1U<<2)

struct apigen_Diagnostics
{
    struct apigen_MemoryArena * arena;
    size_t item_count;
    struct apigen_DiagnosticItem * items;
    uint32_t flags;
};

void apigen_diagnostics_init(struct apigen_Diagnostics * diags, struct apigen_MemoryArena * arena);
void apigen_diagnostics_deinit(struct apigen_Diagnostics * diags);

bool apigen_diagnostics_remove_one(struct apigen_Diagnostics * diags, enum apigen_DiagnosticCode code);
bool apigen_diagnostics_has_any(struct apigen_Diagnostics const * diags);

void apigen_diagnostics_emit(
    struct apigen_Diagnostics * diags,
    char const * file_name,
    uint32_t line_number,
    uint32_t column_number,
    enum apigen_DiagnosticCode code,
    ...);

void apigen_diagnostics_vemit(
    struct apigen_Diagnostics * diags,
    char const * file_name,
    uint32_t line_number,
    uint32_t column_number,
    enum apigen_DiagnosticCode code, 
    va_list list);



void apigen_diagnostics_render(struct apigen_Diagnostics const * diags, struct apigen_Stream stream);

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
extern struct apigen_Type const apigen_type_f32;
extern struct apigen_Type const apigen_type_f64;

