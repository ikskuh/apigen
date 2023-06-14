%{
#include "apigen.h"
#include "parser/parser.h"

typedef void * yyscan_t;
#include "parser.yy.h"
#include "lexer.yy.h"

int yyerror(
    struct apigen_parser_ltype const * location,
    yyscan_t scanner, 
    struct apigen_parser_state * parser_state,
    char const * err_string
);

%}

%locations
%define api.pure full
%param   {yyscan_t yyscanner}
%parse-param {struct apigen_parser_state * parser_state}


%define api.prefix {apigen_parser_}
//%define api.value.type {APIGEN_PARSER_STYPE}
%define api.location.type {struct apigen_parser_ltype}


%start	file 

%token <str> STRING
%token <integer> INTEGER

%token DOCCOMMENT
%token KW_CONST
%token KW_VAR
%token KW_TYPE
%token KW_FN
%token KW_ENUM
%token KW_UNION
%token KW_STRUCT
%token IDENTIFIER

%%

file:
    declaration {}
|   file declaration {}
;

declaration:
    KW_CONST    IDENTIFIER '=' type ';'
|   KW_VAR      IDENTIFIER '=' type ';'
|   KW_TYPE     IDENTIFIER '=' type ';'
|   KW_FN       IDENTIFIER '(' field_list ')' type ';'
|   docs KW_CONST    IDENTIFIER '=' type ';'
|   docs KW_VAR      IDENTIFIER '=' type ';'
|   docs KW_TYPE     IDENTIFIER '=' type ';'
|   docs KW_FN       IDENTIFIER '(' field_list ')' type ';'
;

type:
    IDENTIFIER
|   KW_ENUM   '(' IDENTIFIER ')' '{' enum_items '}'
|   KW_ENUM                      '{' enum_items '}'
|   KW_UNION                     '{' field_list '}'
|   KW_STRUCT                    '{' field_list '}'
;

field_list:
    field_list_inner
|   field_list_inner ','
|
;

field_list_inner:
    field
|   field_list_inner ',' field
;

field: 
    docs IDENTIFIER ':' type
|        IDENTIFIER ':' type
;

enum_items:
    enum_items_inner
|   enum_items_inner ','
;

enum_items_inner:
    enum_item
|   enum_items_inner ',' enum_item
;

enum_item:
    docs IDENTIFIER
|        IDENTIFIER
|   docs IDENTIFIER '=' value
|        IDENTIFIER '=' value
;

docs: DOCCOMMENT
|     DOCCOMMENT docs
;

value: 
    INTEGER
;

%%

#include <stdio.h>
#include <stdlib.h>

int yyerror(struct apigen_parser_ltype const * location, yyscan_t scanner, struct apigen_parser_state * parser_state, char const * err)
{
    fprintf(stderr, "ERROR: %s at symbol '%s' on %s:%d:%d\n",
        err,
        apigen_parser_get_text(scanner),
        parser_state->file_name,
        location->first_line,
        location->first_column
    );
    exit(1);
}
