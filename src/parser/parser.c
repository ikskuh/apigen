#include "apigen.h"
#include "parser.h"

#include "lexer.yy.h"
#include "parser.yy.h"


int apigen_parse (struct apigen_parser_state *state)
{
    APIGEN_NOT_NULL(state);
    
    APIGEN_ASSERT(state->top_level_declarations == NULL);

    {
        yyscan_t scanner;
        apigen_parser_lex_init ( &scanner );
        apigen_parser_set_in(state->file, scanner);
        
        int const parse_ok = apigen_parser_parse ( scanner, state );
    
        apigen_parser_lex_destroy ( scanner );

        if(!parse_ok) {
            return parse_ok;
        }
    }

    APIGEN_ASSERT(state->top_level_declarations != NULL);

    return 0;
}

#include <stdio.h>



struct apigen_value apigen_parser_conv_regular_str(struct apigen_parser_state *state, char const * literal)
{
    APIGEN_NOT_NULL(state);

    size_t const len = strlen(literal);
    APIGEN_ASSERT(len >= 2); // starts and ends with '"'
    APIGEN_ASSERT(literal[0] == '"');
    APIGEN_ASSERT(literal[len - 1] == '"');

    size_t converted_length = 0;
    {
        size_t i = 1;
        while(i < len - 1) {
            char c = literal[i];
            switch(c) {
                case '\\':
                    i += 2;
                    converted_length += 1;
                    break;
                default: 
                    i += 1;
                    converted_length += 1;
                    break;
            }
        }
    }

    char * const converted = apigen_memory_arena_alloc(state->ast_arena, converted_length + 1);
    
    {
        size_t src = 1;
        size_t dst = 0;
        while(src < len - 1) {
            char c = literal[src];
            switch(c) {
                case '\\':
                    c = literal[src+1];
                    switch(c) {
                        case 'n': converted[dst] = '\n'; break;
                        case 'r': converted[dst] = '\r'; break;
                        case 'e': converted[dst] = '\e'; break;
                        case '\"': converted[dst] = '\"'; break;
                        case '\'': converted[dst] = '\''; break;
                        default: converted[dst] = c; break;
                    }
                    src += 2;
                    dst += 1;
                    break;
                default: 
                    converted[dst] = c;
                    src += 1;
                    dst += 1;
                    break;
            }
        }
    }

    converted[converted_length] = 0;
    
    return (struct apigen_value) { .type = apigen_value_str, .value_str = converted };
}

struct apigen_value apigen_parser_conv_multiline_str(struct apigen_parser_state *state, char const * literal)
{
    APIGEN_NOT_NULL(state);

    size_t const len = strlen(literal);
    APIGEN_ASSERT(len >= 2); // starts with '\\'

    char * const converted = apigen_memory_arena_dupestr(state->ast_arena, literal + 2);
    
    return (struct apigen_value) { .type = apigen_value_str, .value_str = converted };
}

struct apigen_value apigen_parser_concat_multiline_strs(struct apigen_parser_state *state, struct apigen_value str1, struct apigen_value str2)
{
    APIGEN_NOT_NULL(state);
    APIGEN_ASSERT(str1.type == apigen_value_str);
    APIGEN_ASSERT(str2.type == apigen_value_str);

    char const * const line_feed = (state->line_feed != NULL) ? state->line_feed : "\n";

    size_t const lf_len = strlen(line_feed);
    size_t const str1_len = strlen(str1.value_str);
    size_t const str2_len = strlen(str2.value_str);

    size_t const total_len = lf_len + str1_len + str2_len + 1;
    
    char * const output_string = apigen_memory_arena_alloc(state->ast_arena, total_len);

    memcpy(output_string,                     str1.value_str, str1_len);
    memcpy(output_string + str1_len,          line_feed, lf_len);
    memcpy(output_string + str1_len + lf_len, str2.value_str, str2_len);
    output_string[total_len] = 0;

    return (struct apigen_value) { .type = apigen_value_str, .value_str = output_string };
}

char const * apigen_parser_concat_doc_strings(struct apigen_parser_state *state, char const * str1, char const * str2)
{
    APIGEN_NOT_NULL(state);
    APIGEN_NOT_NULL(str1);
    APIGEN_NOT_NULL(str2);

    char const * const line_feed = "\n"; // doc strings always are separated by a single "\n"

    size_t const lf_len = strlen(line_feed);
    size_t const str1_len = strlen(str1);
    size_t const str2_len = strlen(str2);

    size_t const total_len = lf_len + str1_len + str2_len + 1;
    
    char * const output_string = apigen_memory_arena_alloc(state->ast_arena, total_len);

    memcpy(output_string,                     str1, str1_len);
    memcpy(output_string + str1_len,          line_feed, lf_len);
    memcpy(output_string + str1_len + lf_len, str2, str2_len);
    output_string[total_len] = 0;

    return output_string;
}

#define DEFINE_LIST_OPERATORS(_ListItem, _Prefix)                                                   \
  _ListItem *_Prefix##_init(struct apigen_parser_state *state, _ListItem item) {                    \
    APIGEN_NOT_NULL(state);                                                                         \
                                                                                                    \
    _ListItem *first = apigen_memory_arena_alloc(state->ast_arena, sizeof(_ListItem));              \
    *first = item;                                                                                  \
    first->next = NULL;                                                                             \
                                                                                                    \
    return first;                                                                                   \
  }                                                                                                 \
                                                                                                    \
  _ListItem *_Prefix##_append(struct apigen_parser_state *state, _ListItem *list, _ListItem item) { \
        APIGEN_NOT_NULL(state);                                                                     \
        APIGEN_NOT_NULL(list);                                                                      \
                                                                                                    \
        _ListItem *new_item = apigen_memory_arena_alloc(state->ast_arena, sizeof(_ListItem));       \
        *new_item = item;                                                                           \
        new_item->next = NULL;                                                                      \
                                                                                                    \
        _ListItem *iter = list;                                                                     \
        while (iter->next != NULL) {                                                                \
            iter = iter->next;                                                                      \
        }                                                                                           \
                                                                                                    \
        APIGEN_ASSERT(iter->next == NULL);                                                          \
                                                                                                    \
        iter->next = new_item;                                                                      \
        return list;                                                                                \
                                                                                                    \
  }

DEFINE_LIST_OPERATORS(struct apigen_parser_enum_item,   apigen_parser_enum_item_list)
DEFINE_LIST_OPERATORS(struct apigen_parser_field,       apigen_parser_field_list)
DEFINE_LIST_OPERATORS(struct apigen_parser_declaration, apigen_parser_file)

struct apigen_parser_type * apigen_parser_heapify_type(struct apigen_parser_state *state, struct apigen_parser_type type)
{
    APIGEN_NOT_NULL(state);

    struct apigen_parser_type * heapified = apigen_memory_arena_alloc(state->ast_arena, sizeof(struct apigen_parser_type));
    *heapified = type;
    return heapified;
}

char const * apigen_parser_conv_at_ident(struct apigen_parser_state *state, char const * at_identifier)
{
    APIGEN_NOT_NULL(state);
    APIGEN_NOT_NULL(at_identifier);

    size_t const at_identifier_len = strlen(at_identifier);
    APIGEN_ASSERT(at_identifier_len >= 3);
    APIGEN_ASSERT(at_identifier[0] == '@');
    APIGEN_ASSERT(at_identifier[1] == '\"');
    APIGEN_ASSERT(at_identifier[at_identifier_len-1] == '\"');

    // Kind of a hack, but also very convenient:
    // @-Identifiers are basically just a regular string literal prefixed with an @, so we
    // can just apply the same conversion rules:
    struct apigen_value converted_name = apigen_parser_conv_regular_str(state, at_identifier + 1); 
    APIGEN_ASSERT(converted_name.type == apigen_value_str);
    return converted_name.value_str;
}