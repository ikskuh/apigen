%{
#include "parser/parser-type.h"
#include "parser.yy.h"
#include "apigen.h"
// #define YYSTYPE char *
%}

%option reentrant stack noyywrap
%option never-interactive

%option nounput
%option noinput


%x COMMENT

%%

"//"                 yy_push_state( COMMENT, yyscanner);
.|\n

<COMMENT>\n          yy_pop_state( yyscanner );
<COMMENT>[^\n]+      fprintf( yyout, "%s\n", yytext);

%%

int apigen_run_lexer ( int argc, char * argv[] )
{
    (void)argc;
    (void)argv;
    // yyscan_t scanner;

    // apigen_parser_lex_init ( &scanner );
    // apigen_parser_lex ( scanner );
    // apigen_parser_lex_destroy ( scanner );

    (void)yy_top_state;
    return 0;
}