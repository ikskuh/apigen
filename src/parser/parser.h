#pragma once
 
struct apigen_parser_ltype
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};

struct apigen_parser_stype {
    int integer;
    char* str;
};

typedef struct apigen_parser_ltype YYLTYPE;
typedef struct apigen_parser_stype YYSTYPE;