
%{
#include "apigen.h"
#include "parser/parser-type.h"

typedef void * yyscan_t;
#include "parser.yy.h"
#include "lexer.yy.h"
int yyerror(struct apigen_parser_ltype const * location, yyscan_t scanner, struct apigen_memory_arena * arena,char const  *s);

%}

%{
/* Pass the argument to yyparse through to yylex. */
#define YYPARSE_PARAM scanner
#define YYLEX_PARAM   scanner
%}

%locations
%define api.pure full
%param   {yyscan_t yyscanner}
%parse-param {struct apigen_memory_arena * arena}


//%define api.prefix {apigen_parser_}
//%define api.value.type {APIGEN_PARSER_STYPE}
%define api.location.type {struct apigen_parser_ltype}


%start	assignment 

%left	PLUS
%left	MULT
%token <str> STRING
%token <num> NUMBER

%%

assignment:
    STRING '=' NUMBER ';' {
        printf( "(setf %s %d)", $1, $3 );
    }
;

%%

#include <stdio.h>
#include <stdlib.h>

int yyerror(struct apigen_parser_ltype const * location, yyscan_t scanner, struct apigen_memory_arena * arena, char const * err)
{
    extern char *yytext;	// defined and maintained in lex.c

    fprintf(stderr, "ERROR: %s at symbol '%s' on line %d:%d\n",
        err,
        yytext,
        location->first_line,
        location->first_column
    );
    exit(1);
}

