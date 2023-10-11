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

// https://ftp.gnu.org/old-gnu/Manuals/flex-2.5.4/html_node/flex_10.html#SEC10
#define YY_INPUT(buf, result, max_size)                                             \
    {                                                                               \
        struct apigen_ParserState * const parser_state = yyget_extra(yyscanner);    \
        APIGEN_NOT_NULL(parser_state);                                              \
        size_t count = apigen_io_read(parser_state->file, buf, max_size);           \
        result = (count > 0) ? count : YY_NULL ;                                    \
    }

typedef struct apigen_ParserLocation YYLTYPE;
typedef union  apigen_ParserAstNode YYSTYPE;
%}


/* generate 8-bit scanner */
%option 8bit                  
/* generate batch scanner */
%option batch                 
/* track line count in yylineno */
%option yylineno              
/* custom prefix instead of yy */
%option prefix="apigen_parser_"
/* generate a reentrant C scanner */
%option reentrant             
/* do not include <unistd.h> */
%option nounistd              
/* include yylloc support. */
%option bison-locations       
/* scanner for bison pure parser. */
%option bison-bridge          

/* supress generation of main() function */
%option nomain                
%option nodefault
/* enables the use of start condition stacks (see Start Conditions). */
%option stack                 
/* instructs flex to generate a scanner which never considers its input interactive. */
%option never-interactive     

%option nounput
%option noinput

/* files cannot wrap into other files */
%option noyywrap

ident   [A-Za-z_][A-Za-z0-9_]*
atident @\"[^\"]+\"

dec_uint [0-9_]+
hex_uint 0x[0-9a-fA-F_]+
bin_uint 0b[01_]+

dec_sint -[0-9_]+
hex_sint -0x[0-9a-fA-F_]+
bin_sint -0b[01_]+

string_lit  \"([^\"\\\n]|\\.)*\"

multiline_str   \\\\[^\n]*

%%

"///"[^\n]*     { yylval->plain_text = apigen_parser_create_doc_string(parser_state, yytext); return DOCCOMMENT; }

"//"[^\n]*      { }

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
opaque          { return KW_OPAQUE; }
include         { return KW_INCLUDE; }

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

{hex_uint}      { yylval->value = (struct apigen_Value) { .type = apigen_value_uint, .value_uint = apigen_parse_uint(yytext+2, 16) }; return INTEGER; }
{bin_uint}      { yylval->value = (struct apigen_Value) { .type = apigen_value_uint, .value_uint = apigen_parse_uint(yytext+2,  2) }; return INTEGER; }
{dec_uint}      { yylval->value = (struct apigen_Value) { .type = apigen_value_uint, .value_uint = apigen_parse_uint(yytext+0, 10) }; return INTEGER; }
{hex_sint}      { yylval->value = (struct apigen_Value) { .type = apigen_value_sint, .value_sint = apigen_parse_sint(yytext+3, 16) }; return INTEGER; }
{bin_sint}      { yylval->value = (struct apigen_Value) { .type = apigen_value_sint, .value_sint = apigen_parse_sint(yytext+3,  2) }; return INTEGER; }
{dec_sint}      { yylval->value = (struct apigen_Value) { .type = apigen_value_sint, .value_sint = apigen_parse_sint(yytext+1, 10) }; return INTEGER; }
null            { yylval->value = (struct apigen_Value) { .type = apigen_value_null };                  return NULLVAL; }

{ident}         { yylval->identifier = apigen_memory_arena_dupestr(parser_state->ast_arena, yytext); return IDENTIFIER; }

[ \r\n]         { }
.               { printf("unexpected char: '%s'", yytext); return -1; }

%%
