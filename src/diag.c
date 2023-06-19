#include "apigen.h"

#include <string.h>

struct apigen_diagnostic_item 
{
    struct apigen_diagnostic_item * next;

    enum apigen_diagnostic_code code;
    
    char const * message;
    char const * file_name;
    int line, column;
};

static bool is_in_range(int lo, int hi, int val)
{
    return (val >= lo) && (val <= hi);
}

static uint32_t classify_diag_code(enum apigen_diagnostic_code code)
{
    if(is_in_range(APIGEN_DIAGNOSTIC_FIRST_ERR, APIGEN_DIAGNOSTIC_LAST_ERR, code)) {
        return APIGEN_DIAGNOSTIC_FLAG_ERROR;
    }
    else if(is_in_range(APIGEN_DIAGNOSTIC_FIRST_WARN, APIGEN_DIAGNOSTIC_LAST_WARN, code)) {
        return APIGEN_DIAGNOSTIC_FLAG_WARN;
    }
    else if(is_in_range(APIGEN_DIAGNOSTIC_FIRST_NOTE, APIGEN_DIAGNOSTIC_LAST_NOTE, code)) {
        return APIGEN_DIAGNOSTIC_FLAG_NOTE;
    }
    else {
        apigen_panic("encountered illegal diagnostic code");
    }
}

static char const * get_diag_code_fmt_string(enum apigen_diagnostic_code code)
{
    switch(code) {
#define APIGEN_TEMP_MACRO(_Symbol, _Id, _Format) case _Symbol:  return _Format;
APIGEN_EXPAND_DIAGNOSTIC_CODE_SET(APIGEN_TEMP_MACRO)
#undef  APIGEN_TEMP_MACRO

        default: apigen_panic("Error code was not added to format list");
    }
}

void apigen_diagnostics_emit(
        struct apigen_Diagnostics * diags, 
        char const * file_name,
        uint32_t line_number,
        uint32_t column_number,
        enum apigen_diagnostic_code code, 
        ...
    )
{
    va_list list;
    va_start(list, code);
    apigen_diagnostics_vemit(diags, file_name, line_number, column_number, code, list);
    va_end(list);
}

void apigen_diagnostics_vemit(
        struct apigen_Diagnostics * diags, 
        char const * file_name,
        uint32_t line_number,
        uint32_t column_number,
        enum apigen_diagnostic_code code, 
        va_list src_list
    )
{
    APIGEN_NOT_NULL(diags);
    APIGEN_NOT_NULL(diags->arena);

    char const * const message_format_str = get_diag_code_fmt_string(code);
    APIGEN_ASSERT(message_format_str != NULL);

    int formatted_message_len;
    {
        va_list list;
        va_copy(list, src_list);
        formatted_message_len = vsnprintf(NULL, 0, message_format_str, list);
        va_end(list);
        APIGEN_ASSERT(formatted_message_len >= 0);
    }

    char * const formatted_message = apigen_memory_arena_alloc(diags->arena, (size_t)formatted_message_len + 1);
    {
        va_list list;
        va_copy(list, src_list);
        int length = vsnprintf(formatted_message, formatted_message_len + 1, message_format_str, list);
        va_end(list);
        APIGEN_ASSERT(length == formatted_message_len);
        formatted_message[formatted_message_len] = 0; // ensure NULL terminator
    }

    struct apigen_diagnostic_item * const item = apigen_memory_arena_alloc(diags->arena, sizeof(struct apigen_diagnostic_item));
    *item = (struct apigen_diagnostic_item) {
        .next = diags->items,

        .code = code,
        .message = formatted_message,

        .file_name = apigen_memory_arena_dupestr(diags->arena, file_name),
        .line = line_number,
        .column = column_number,
    };

    diags->items = item;
    diags->item_count += 1;

    diags->flags |= classify_diag_code(code);
}

void apigen_diagnostics_render(struct apigen_Diagnostics const * diags, struct apigen_Stream stream)
{
    APIGEN_NOT_NULL(diags);

    struct apigen_diagnostic_item const * iter = diags->items;
    while(iter)
    {
        char const * type_kind = NULL;
        switch(classify_diag_code(iter->code)) {
            case APIGEN_DIAGNOSTIC_FLAG_ERROR: type_kind = "error"; break;
            case APIGEN_DIAGNOSTIC_FLAG_WARN:  type_kind = "warn";  break;
            case APIGEN_DIAGNOSTIC_FLAG_NOTE:  type_kind = "note";  break;
            default: apigen_panic("unhandled diagnostic code");
        }
        APIGEN_ASSERT(type_kind != NULL);

        apigen_io_printf(
            stream,
            "%s:%d:%d: %s(%d): %s\n", // magic.api:44:3: error(2003): file not found
            iter->file_name,
            iter->line + 1,
            iter->column + 1,
            type_kind,
            (int)iter->code,
            iter->message
        );
        iter = iter->next;
    }
}

void apigen_diagnostics_init(struct apigen_Diagnostics * diags, struct apigen_MemoryArena * arena)
{
    APIGEN_NOT_NULL(diags);
    APIGEN_NOT_NULL(arena);
    *diags = (struct apigen_Diagnostics) {
        .arena = arena,
    };
}

void apigen_diagnostics_deinit(struct apigen_Diagnostics * diags)
{
    APIGEN_NOT_NULL(diags);
    memset(diags, 0xAA, sizeof(struct apigen_Diagnostics));
}
