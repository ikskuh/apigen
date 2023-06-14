#include "apigen.h"
#include "parser.h"

#include "lexer.yy.h"
#include "parser.yy.h"


int apigen_parse (struct apigen_parser_state *state)
{
    (void)state;

    yyscan_t scanner;

    apigen_parser_lex_init ( &scanner );

    apigen_parser_set_in(state->file, scanner);

    int ok = apigen_parser_parse ( scanner, state );
    
    apigen_parser_lex_destroy ( scanner );

    return ok;
}
