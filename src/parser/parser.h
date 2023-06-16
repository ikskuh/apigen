#pragma once

#include "apigen.h"
 
struct apigen_ParserLType
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};

#define APIGEN_VALUE_NULL ((struct apigen_Value){ .type = apigen_value_null })

struct apigen_ParserEnumItem;
struct apigen_ParserField;

enum apigen_ParserTypeId {
    apigen_parser_type_named,
    apigen_parser_type_enum,
    apigen_parser_type_struct,
    apigen_parser_type_union,
    apigen_parser_type_array,
    apigen_parser_type_ptr_to_one,
    apigen_parser_type_ptr_to_many,
    apigen_parser_type_ptr_to_many_sentinelled,
    apigen_parser_type_function,
    apigen_parser_type_opaque,
};

struct apigen_ParserType {
    enum apigen_ParserTypeId type;
    union {
        struct {
            struct apigen_ParserType * underlying_type;
            struct apigen_ParserEnumItem * items;
        } enum_data;
        struct apigen_ParserField * union_struct_fields;
        char const * named_data;
        struct {
            struct apigen_ParserType * underlying_type;
            struct apigen_Value size;
        } array_data;
        struct {
            struct apigen_ParserType * underlying_type;
            bool is_const;
            bool is_optional;
            struct apigen_Value sentinel;
        } pointer_data;
        struct {
            struct apigen_ParserType * return_type;
            struct apigen_ParserField * parameters;
        } function_data;
    };
};  

struct apigen_ParserEnumItem {
    char const * documentation;
    char const * identifier;
    struct apigen_Value value;
    struct apigen_ParserEnumItem * next;
};

struct apigen_ParserField {
    char const * documentation;
    char const * identifier;
    struct apigen_ParserType type;
    struct apigen_ParserField * next;
};

enum apigen_ParserDeclarationKind {
    apigen_parser_const_declaration,
    apigen_parser_var_declaration,
    apigen_parser_constexpr_declaration,
    apigen_parser_fn_declaration,
    apigen_parser_type_declaration,
};

struct apigen_ParserDeclaration {
    enum apigen_ParserDeclarationKind kind;
    char const * documentation;
    char const * identifier;
    struct apigen_ParserType type;
    struct apigen_Value       initial_value;
    struct apigen_ParserDeclaration * next;

    struct apigen_Type * associated_type;
};

union apigen_ParserAstNode {
    struct apigen_Value value;
    char const * identifier;
    char const * plain_text;
    struct apigen_ParserEnumItem enum_item;
    struct apigen_ParserEnumItem * enum_item_list;
    struct apigen_ParserField field;
    struct apigen_ParserField * field_list;
    struct apigen_ParserType type;
    struct apigen_ParserDeclaration declaration;
    struct apigen_ParserDeclaration * file;
};

typedef struct apigen_ParserLType YYLTYPE;
typedef union apigen_ParserAstNode YYSTYPE;


struct apigen_Value apigen_parser_conv_regular_str(struct apigen_ParserState *state, char const * literal);
struct apigen_Value apigen_parser_conv_multiline_str(struct apigen_ParserState *state, char const * literal);
struct apigen_Value apigen_parser_concat_multiline_strs(struct apigen_ParserState *state, struct apigen_Value str1, struct apigen_Value str2);

char const * apigen_parser_concat_doc_strings(struct apigen_ParserState *state, char const * str1, char const * str2);

struct apigen_ParserEnumItem * apigen_parser_enum_item_list_init(struct apigen_ParserState *state, struct apigen_ParserEnumItem item);
struct apigen_ParserEnumItem * apigen_parser_enum_item_list_append(struct apigen_ParserState *state, struct apigen_ParserEnumItem * list, struct apigen_ParserEnumItem item);

struct apigen_ParserField * apigen_parser_field_list_init(struct apigen_ParserState *state, struct apigen_ParserField item);
struct apigen_ParserField * apigen_parser_field_list_append(struct apigen_ParserState *state, struct apigen_ParserField * list, struct apigen_ParserField item);

struct apigen_ParserDeclaration * apigen_parser_file_init(struct apigen_ParserState *state, struct apigen_ParserDeclaration item);
struct apigen_ParserDeclaration * apigen_parser_file_append(struct apigen_ParserState *state, struct apigen_ParserDeclaration * list, struct apigen_ParserDeclaration item);


char const * apigen_parser_conv_at_ident(struct apigen_ParserState *state, char const * at_identifier);

struct apigen_ParserType * apigen_parser_heapify_type(struct apigen_ParserState *state, struct apigen_ParserType type);
