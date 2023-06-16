%{
#include "apigen.h"
#include "parser/parser.h"

typedef void * yyscan_t;
#include "parser.yy.h"

#define YY_DECL int yylex( \
    YYSTYPE * yylval_param, \
    YYLTYPE * yylloc_param , \
    yyscan_t yyscanner, \
    struct apigen_ParserState * parser_state \
)
extern YY_DECL;

#include "lexer.yy.h"

int yyerror(
    struct apigen_ParserLType const * location,
    yyscan_t scanner, 
    struct apigen_ParserState * parser_state,
    char const * err_string
);

%}

%locations
%define api.pure full
%param   {yyscan_t yyscanner}
%parse-param {struct apigen_ParserState * parser_state}
%lex-param {struct apigen_ParserState * parser_state}

%define api.prefix {apigen_parser_}
%define api.value.type {union apigen_ParserAstNode}
%define api.location.type {struct apigen_ParserLType}


%start	file 

%token <value> STRING
%token <value> MULTILINE_STRING
%token <value> INTEGER
%token <value> NULLVAL
%token <identifier> IDENTIFIER
%token <plain_text> DOCCOMMENT

%token KW_CONST
%token KW_VAR
%token KW_TYPE
%token KW_FN
%token KW_ENUM
%token KW_UNION
%token KW_STRUCT
%token KW_CONSTEXPR
%token KW_OPAQUE

%type <plain_text> docs
%type <value> value
%type <value> multiline_str


%type <enum_item>      enum_item
%type <enum_item_list> enum_items       // returns linked-list to enum_item
%type <enum_item_list> enum_items_inner // returns linked-list to enum_item

%type <field>      field
%type <field_list> field_list           // returns linked-list to field
%type <field_list> field_list_inner     // returns linked-list to field

%type <type> type
%type <type> primitive_type
%type <type> named_type
%type <type> function_type
%type <type> function_signature

%type <declaration> declaration
%type <file>        declaration_list

%%

file:
    declaration_list { parser_state->top_level_declarations = $1; }
;

declaration_list:
    declaration                  { $$ = apigen_parser_file_init(parser_state, $1); }
|   declaration_list declaration { $$ = apigen_parser_file_append(parser_state, $1, $2); }
;

declaration:
         KW_CONST     IDENTIFIER ':' type ';'            { $$ = (struct apigen_ParserDeclaration) { .kind = apigen_parser_const_declaration, .documentation = NULL, .identifier = $2, .type = $4 }; }
|        KW_VAR       IDENTIFIER ':' type ';'            { $$ = (struct apigen_ParserDeclaration) { .kind = apigen_parser_var_declaration,   .documentation = NULL, .identifier = $2, .type = $4 }; }
|        KW_TYPE      IDENTIFIER '=' type ';'            { $$ = (struct apigen_ParserDeclaration) { .kind = apigen_parser_type_declaration,  .documentation = NULL, .identifier = $2, .type = $4 }; }
|        KW_CONSTEXPR IDENTIFIER ':' type '=' value ';'  { $$ = (struct apigen_ParserDeclaration) { .kind = apigen_parser_type_declaration,  .documentation = NULL, .identifier = $2, .type = $4, .initial_value = $6 }; }
|        KW_FN        IDENTIFIER function_signature ';'  { $$ = (struct apigen_ParserDeclaration) { .kind = apigen_parser_type_declaration,  .documentation = NULL, .identifier = $2, .type = $3 }; }
|   docs KW_CONST     IDENTIFIER '=' type ';'            { $$ = (struct apigen_ParserDeclaration) { .kind = apigen_parser_const_declaration, .documentation = $1,   .identifier = $3, .type = $5 }; }
|   docs KW_VAR       IDENTIFIER '=' type ';'            { $$ = (struct apigen_ParserDeclaration) { .kind = apigen_parser_var_declaration,   .documentation = $1,   .identifier = $3, .type = $5 }; }
|   docs KW_TYPE      IDENTIFIER '=' type ';'            { $$ = (struct apigen_ParserDeclaration) { .kind = apigen_parser_type_declaration,  .documentation = $1,   .identifier = $3, .type = $5 }; }
|   docs KW_CONSTEXPR IDENTIFIER ':' type '=' value ';'  { $$ = (struct apigen_ParserDeclaration) { .kind = apigen_parser_type_declaration,  .documentation = $1,   .identifier = $3, .type = $5, .initial_value = $7 }; }
|   docs KW_FN        IDENTIFIER function_signature ';'  { $$ = (struct apigen_ParserDeclaration) { .kind = apigen_parser_type_declaration,  .documentation = $1,   .identifier = $3, .type = $4 }; }
;

type:
    primitive_type
|   function_type
|   KW_ENUM   '(' named_type ')' '{' enum_items '}'  { $$ = (struct apigen_ParserType) { .type = apigen_parser_type_enum, .enum_data = { .underlying_type = apigen_parser_heapify_type(parser_state, $3), .items = $6 } }; }
|   KW_ENUM                      '{' enum_items '}'  { $$ = (struct apigen_ParserType) { .type = apigen_parser_type_enum, .enum_data = { .underlying_type = NULL,                                         .items = $3 } }; }
|   KW_UNION                     '{' field_list '}'  { $$ = (struct apigen_ParserType) { .type = apigen_parser_type_union,  .union_struct_fields = $3 }; }
|   KW_STRUCT                    '{' field_list '}'  { $$ = (struct apigen_ParserType) { .type = apigen_parser_type_struct, .union_struct_fields = $3 }; }
|   KW_OPAQUE                    '{' '}'             { $$ = (struct apigen_ParserType) { .type = apigen_parser_type_opaque }; }
;

function_type:
    KW_FN function_signature { $$ = $2; }
;

function_signature:
    '(' field_list ')' type { $$ = (struct apigen_ParserType) { .type = apigen_parser_type_function, .function_data = { .parameters = $2, .return_type = apigen_parser_heapify_type(parser_state, $4) } }; }
;

primitive_type:
    '*'                       KW_CONST type { $$ = (struct apigen_ParserType) { .type = apigen_parser_type_ptr_to_one,              .pointer_data = { .underlying_type = apigen_parser_heapify_type(parser_state, $3), .is_const = true,  .is_optional = false, .sentinel = APIGEN_VALUE_NULL } }; }
|       '[' '*' ']'           KW_CONST type { $$ = (struct apigen_ParserType) { .type = apigen_parser_type_ptr_to_many,             .pointer_data = { .underlying_type = apigen_parser_heapify_type(parser_state, $5), .is_const = true,  .is_optional = false, .sentinel = APIGEN_VALUE_NULL } }; }
|       '[' '*' ':' value ']' KW_CONST type { $$ = (struct apigen_ParserType) { .type = apigen_parser_type_ptr_to_many_sentinelled, .pointer_data = { .underlying_type = apigen_parser_heapify_type(parser_state, $7), .is_const = true,  .is_optional = false, .sentinel = $4 } }; }
|   '?' '*'                   KW_CONST type { $$ = (struct apigen_ParserType) { .type = apigen_parser_type_ptr_to_one,              .pointer_data = { .underlying_type = apigen_parser_heapify_type(parser_state, $4), .is_const = true,  .is_optional = true,  .sentinel = APIGEN_VALUE_NULL } }; }
|   '?' '[' '*' ']'           KW_CONST type { $$ = (struct apigen_ParserType) { .type = apigen_parser_type_ptr_to_many,             .pointer_data = { .underlying_type = apigen_parser_heapify_type(parser_state, $6), .is_const = true,  .is_optional = true,  .sentinel = APIGEN_VALUE_NULL } }; }
|   '?' '[' '*' ':' value ']' KW_CONST type { $$ = (struct apigen_ParserType) { .type = apigen_parser_type_ptr_to_many_sentinelled, .pointer_data = { .underlying_type = apigen_parser_heapify_type(parser_state, $8), .is_const = true,  .is_optional = true,  .sentinel = $5 } }; }
|       '*'                            type { $$ = (struct apigen_ParserType) { .type = apigen_parser_type_ptr_to_one,              .pointer_data = { .underlying_type = apigen_parser_heapify_type(parser_state, $2), .is_const = false, .is_optional = false, .sentinel = APIGEN_VALUE_NULL } }; }
|       '[' '*' ']'                    type { $$ = (struct apigen_ParserType) { .type = apigen_parser_type_ptr_to_many,             .pointer_data = { .underlying_type = apigen_parser_heapify_type(parser_state, $4), .is_const = false, .is_optional = false, .sentinel = APIGEN_VALUE_NULL } }; }
|       '[' '*' ':' value ']'          type { $$ = (struct apigen_ParserType) { .type = apigen_parser_type_ptr_to_many_sentinelled, .pointer_data = { .underlying_type = apigen_parser_heapify_type(parser_state, $6), .is_const = false, .is_optional = false, .sentinel = $4 } }; }
|   '?' '*'                            type { $$ = (struct apigen_ParserType) { .type = apigen_parser_type_ptr_to_one,              .pointer_data = { .underlying_type = apigen_parser_heapify_type(parser_state, $3), .is_const = false, .is_optional = true,  .sentinel = APIGEN_VALUE_NULL } }; }
|   '?' '[' '*' ']'                    type { $$ = (struct apigen_ParserType) { .type = apigen_parser_type_ptr_to_many,             .pointer_data = { .underlying_type = apigen_parser_heapify_type(parser_state, $5), .is_const = false, .is_optional = true,  .sentinel = APIGEN_VALUE_NULL } }; }
|   '?' '[' '*' ':' value ']'          type { $$ = (struct apigen_ParserType) { .type = apigen_parser_type_ptr_to_many_sentinelled, .pointer_data = { .underlying_type = apigen_parser_heapify_type(parser_state, $7), .is_const = false, .is_optional = true,  .sentinel = $5 } }; }
|   '[' value ']'                      type { $$ = (struct apigen_ParserType) { .type = apigen_parser_type_array, .array_data = { .underlying_type = apigen_parser_heapify_type(parser_state, $4), .size = $2 } }; }
|   named_type
;

named_type: IDENTIFIER { $$ = (struct apigen_ParserType) { .type = apigen_parser_type_named, .named_data = $1 }; }

field_list:
    field_list_inner      { $$ = $1; }
|   field_list_inner ','  { $$ = $1; }
|                         { $$ = NULL; }
; 

field_list_inner:
    field                       { $$ = apigen_parser_field_list_init(parser_state, $1); }
|   field_list_inner ',' field  { $$ = apigen_parser_field_list_append(parser_state, $1, $3); }
;

field: 
    docs IDENTIFIER ':' type    { $$ = (struct apigen_ParserField) { .documentation = $1,   .identifier = $2, .type = $4 }; }
|        IDENTIFIER ':' type    { $$ = (struct apigen_ParserField) { .documentation = NULL, .identifier = $1, .type = $3 }; }
;

enum_items:
    enum_items_inner      { $$ = $1; }
|   enum_items_inner ','  { $$ = $1; }
;

enum_items_inner:
    enum_item                       { $$ = apigen_parser_enum_item_list_init(parser_state, $1); }
|   enum_items_inner ',' enum_item  { $$ = apigen_parser_enum_item_list_append(parser_state, $1, $3); }
;

enum_item:
    docs IDENTIFIER            { $$ = (struct apigen_ParserEnumItem) { .documentation = $1,   .identifier = $2, .value = APIGEN_VALUE_NULL }; }
|        IDENTIFIER            { $$ = (struct apigen_ParserEnumItem) { .documentation = NULL, .identifier = $1, .value = APIGEN_VALUE_NULL }; }
|   docs IDENTIFIER '=' value  { $$ = (struct apigen_ParserEnumItem) { .documentation = $1,   .identifier = $2, .value = $4 }; }
|        IDENTIFIER '=' value  { $$ = (struct apigen_ParserEnumItem) { .documentation = NULL, .identifier = $1, .value = $3 }; }
;

docs: DOCCOMMENT
|     DOCCOMMENT docs { $$ = apigen_parser_concat_doc_strings(parser_state, $1, $2); } 
;

value: 
    NULLVAL
|   INTEGER
|   STRING
|   multiline_str
;

multiline_str:
    MULTILINE_STRING
|   MULTILINE_STRING multiline_str { $$ = apigen_parser_concat_multiline_strs(parser_state, $1, $2); }
;

%%

#include <stdio.h>
#include <stdlib.h>

int yyerror(struct apigen_ParserLType const * location, yyscan_t scanner, struct apigen_ParserState * parser_state, char const * err)
{
    fprintf(stderr, "ERROR: %s at symbol '%s' on %s:%d:%d\n",
        err,
        apigen_parser_get_text(scanner),
        parser_state->file_name,
        location->first_line + 1,
        location->first_column + 1
    );
    exit(1);
}
