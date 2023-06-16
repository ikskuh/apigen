%{
#include "apigen.h"

#define YY_DECL int yylex( \
    YYSTYPE * yylval_param, \
    YYLTYPE * yylloc_param , \
    yyscan_t yyscanner, \
    struct apigen_ParserState * parser_state \
)

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


typedef struct apigen_ParserLType YYLTYPE;
typedef union  apigen_ParserAstNode YYSTYPE;
%}

%option reentrant stack noyywrap
%option never-interactive

%option nounput
%option noinput
%option yylineno

ident   [A-Za-z_][A-Za-z0-9_]*
atident @\"[^\"]+\"

dec_uint [0-9_]+
hex_uint 0x[0-9_]+
bin_uint 0b[0-9_]+

dec_sint -[0-9_]+
hex_sint -0x[0-9_]+
bin_sint -0b[0-9_]+

string_lit  \"([^\"\\\n]|\\.)*\"

multiline_str   \\\\[^\n]*

%%

"///"[^\n]* { return DOCCOMMENT; }

"//"[^\n]*  { }

{string_lit}    { yylval->value = apigen_parser_conv_regular_str(parser_state, yytext); return STRING; }
{multiline_str} { yylval->value = apigen_parser_conv_multiline_str(parser_state, yytext); return MULTILINE_STRING; }

{atident}       { yylval->identifier = apigen_parser_conv_at_ident(parser_state, yytext); return IDENTIFIER; }

const           { return KW_CONST; }
constexpr       { return KW_CONSTEXPR; }
var             { return KW_VAR; }
type            { return KW_TYPE; }
fn              { return KW_FN; }
enum            { return KW_ENUM; }
union           { return KW_UNION; }
struct          { return KW_STRUCT; }

"*"             { return yytext[0]; }
"["             { return yytext[0]; }
"]"             { return yytext[0]; }
"("             { return yytext[0]; }
")"             { return yytext[0]; }
"{"             { return yytext[0]; }
"}"             { return yytext[0]; }
"="             { return yytext[0]; }
";"             { return yytext[0]; }
","             { return yytext[0]; }
":"             { return yytext[0]; }
"?"             { return yytext[0]; }

{hex_uint}      { yylval->value = (struct apigen_Value) { .type = apigen_value_uint, .value_uint = 0 }; return INTEGER; }
{bin_uint}      { yylval->value = (struct apigen_Value) { .type = apigen_value_uint, .value_uint = 0 }; return INTEGER; }
{dec_uint}      { yylval->value = (struct apigen_Value) { .type = apigen_value_uint, .value_uint = 0 }; return INTEGER; }
{hex_sint}      { yylval->value = (struct apigen_Value) { .type = apigen_value_sint, .value_sint = 0 }; return INTEGER; }
{bin_sint}      { yylval->value = (struct apigen_Value) { .type = apigen_value_sint, .value_sint = 0 }; return INTEGER; }
{dec_sint}      { yylval->value = (struct apigen_Value) { .type = apigen_value_sint, .value_sint = 0 }; return INTEGER; }
null            { yylval->value = (struct apigen_Value) { .type = apigen_value_null };                  return NULLVAL; }


{ident}         { yylval->identifier = apigen_memory_arena_dupestr(parser_state->ast_arena, yytext); return IDENTIFIER; }

[ \r\n]         { }
.               { printf("unexpected char: '%s'", yytext); return -1; }

%%
