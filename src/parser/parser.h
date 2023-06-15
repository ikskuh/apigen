#pragma once
 
struct apigen_parser_ltype
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};


enum apigen_value_type {
    apigen_value_null,
    apigen_value_sint,
    apigen_value_uint,
    apigen_value_str,
};

struct apigen_value {
    enum apigen_value_type type;
    union {
        uint64_t value_uint;
        int64_t value_sint;
        char const * value_str;
    };
};

#define APIGEN_VALUE_NULL ((struct apigen_value){ .type = apigen_value_null })

struct apigen_parser_enum_item;
struct apigen_parser_field;

enum apigen_parser_type_id {
    apigen_parser_type_named,
    apigen_parser_type_enum,
    apigen_parser_type_struct,
    apigen_parser_type_union,
    apigen_parser_type_array,
    apigen_parser_type_ptr_to_one,
    apigen_parser_type_ptr_to_many,
    apigen_parser_type_ptr_to_many_sentinelled,
    apigen_parser_type_function,
};

struct apigen_parser_type {
    enum apigen_parser_type_id type;
    union {
        struct {
            struct apigen_parser_type * underlying_type;
            struct apigen_parser_enum_item * items;
        } enum_data;
        struct apigen_parser_field * union_struct_fields;
        char const * named_data;
        struct {
            struct apigen_parser_type * underlying_type;
            struct apigen_value size;
        } array_data;
        struct {
            struct apigen_parser_type * underlying_type;
            bool is_const;
            bool is_optional;
            struct apigen_value sentinel;
        } pointer_data;
        struct {
            struct apigen_parser_type * return_type;
            struct apigen_parser_field * parameters;
        } function_data;
    };
};  

struct apigen_parser_enum_item {
    char const * documentation;
    char const * identifier;
    struct apigen_value value;
    struct apigen_parser_enum_item * next;
};

struct apigen_parser_field {
    char const * documentation;
    char const * identifier;
    struct apigen_parser_type type;
    struct apigen_parser_field * next;
};

enum apigen_parser_declaration_kind {
    apigen_parser_const_declaration,
    apigen_parser_var_declaration,
    apigen_parser_constexpr_declaration,
    apigen_parser_fn_declaration,
    apigen_parser_type_declaration,
};

struct apigen_parser_declaration {
    enum apigen_parser_declaration_kind kind;
    char const * documentation;
    char const * identifier;
    struct apigen_parser_type type;
    struct apigen_value       initial_value;
    struct apigen_parser_declaration * next;
};

union apigen_parser_stype {
    struct apigen_value value;
    char const * identifier;
    char const * plain_text;
    struct apigen_parser_enum_item enum_item;
    struct apigen_parser_enum_item * enum_item_list;
    struct apigen_parser_field field;
    struct apigen_parser_field * field_list;
    struct apigen_parser_type type;
    struct apigen_parser_declaration declaration;
    struct apigen_parser_declaration * file;
};

typedef struct apigen_parser_ltype YYLTYPE;
typedef union apigen_parser_stype YYSTYPE;


struct apigen_value apigen_parser_conv_regular_str(struct apigen_parser_state *state, char const * literal);
struct apigen_value apigen_parser_conv_multiline_str(struct apigen_parser_state *state, char const * literal);
struct apigen_value apigen_parser_concat_multiline_strs(struct apigen_parser_state *state, struct apigen_value str1, struct apigen_value str2);

char const * apigen_parser_concat_doc_strings(struct apigen_parser_state *state, char const * str1, char const * str2);

struct apigen_parser_enum_item * apigen_parser_enum_item_list_init(struct apigen_parser_state *state, struct apigen_parser_enum_item item);
struct apigen_parser_enum_item * apigen_parser_enum_item_list_append(struct apigen_parser_state *state, struct apigen_parser_enum_item * list, struct apigen_parser_enum_item item);

struct apigen_parser_field * apigen_parser_field_list_init(struct apigen_parser_state *state, struct apigen_parser_field item);
struct apigen_parser_field * apigen_parser_field_list_append(struct apigen_parser_state *state, struct apigen_parser_field * list, struct apigen_parser_field item);

struct apigen_parser_declaration * apigen_parser_file_init(struct apigen_parser_state *state, struct apigen_parser_declaration item);
struct apigen_parser_declaration * apigen_parser_file_append(struct apigen_parser_state *state, struct apigen_parser_declaration * list, struct apigen_parser_declaration item);


char const * apigen_parser_conv_at_ident(struct apigen_parser_state *state, char const * at_identifier);

struct apigen_parser_type * apigen_parser_heapify_type(struct apigen_parser_state *state, struct apigen_parser_type type);
