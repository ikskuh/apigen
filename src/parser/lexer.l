%{
#include "apigen.h"
#include "parser/parser.h"
#include "parser.yy.h"
#include "apigen.h"
#include <string.h>

static inline char * strdup(char const * string)
{
    size_t len = strlen(string);
    char * dupe = malloc(len + 1);
    if(dupe == NULL) {
        return NULL;
    }
    memcpy(dupe, string, len);
    dupe[len] = 0;
    return dupe;
}

#define YY_USER_ACTION \
    yylloc->first_line = yylloc->last_line; \
    yylloc->first_column = yylloc->last_column; \
    for(int i = 0; yytext[i] != '\0'; i++) { \
        if(yytext[i] == '\n') { \
            yylloc->last_line++; \
            yylloc->last_column = 0; \
        } \
        else { \
            yylloc->last_column++; \
        } \
    }
%}

%option reentrant stack noyywrap
%option never-interactive

%option nounput
%option noinput
%option yylineno

ident   [A-Za-z_][A-Za-z0-9_]*
dec_int -?[0-9_]+
hex_int -?0x[0-9_]+
bin_int -?0b[0-9_]+


%%

"///"[^\n]* { return DOCCOMMENT; }

const       { return KW_CONST; }
var         { return KW_VAR; }
type        { return KW_TYPE; }
fn          { return KW_FN; }
enum        { return KW_ENUM; }
union       { return KW_UNION; }
struct      { return KW_STRUCT; }

"*"         { return yytext[0]; }
"["         { return yytext[0]; }
"]"         { return yytext[0]; }
"("         { return yytext[0]; }
")"         { return yytext[0]; }
"{"         { return yytext[0]; }
"}"         { return yytext[0]; }
"="         { return yytext[0]; }
";"         { return yytext[0]; }
","         { return yytext[0]; }
":"         { return yytext[0]; }

{ident}     { return IDENTIFIER; }
{hex_int}   { return INTEGER; }
{bin_int}   { return INTEGER; }
{dec_int}   { return INTEGER; }

[ \r\n]     { }
.           { printf("unexpected char: '%s'", yytext); return -1; }

%%
// "//"[^\n]*  { }