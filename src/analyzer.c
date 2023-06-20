#include "apigen.h"
#include "parser/parser.h"

#include <stdio.h>
#include <setjmp.h>
#include <stdint.h>
#include <limits.h>
#include <inttypes.h>

static void emit_diagnostics(
    struct apigen_ParserState * const parser,
    struct apigen_ParserLocation location,
    enum apigen_diagnostic_code code,
    ...)
{
    va_list list;
    va_start(list, code);
    apigen_diagnostics_vemit(
        parser->diagnostics,
        parser->file_name,
        location.first_line,
        location.first_column,
        code,
        list
    );
    va_end(list);
}

static bool is_unique_type(enum apigen_ParserTypeId id) {
    switch(id) {
        case apigen_parser_type_named:                      return false;
        case apigen_parser_type_enum:                       return true;
        case apigen_parser_type_struct:                     return true;
        case apigen_parser_type_union:                      return true;
        case apigen_parser_type_opaque:                     return true;
        case apigen_parser_type_array:                      return false;
        case apigen_parser_type_ptr_to_one:                 return false;
        case apigen_parser_type_ptr_to_many:                return false;
        case apigen_parser_type_ptr_to_many_sentinelled:    return false;
        case apigen_parser_type_function:                   return false;
    }
}

static bool is_integer_type(struct apigen_Type type)
{
    switch(type.id)
    {
        case apigen_typeid_uchar:
        case apigen_typeid_ichar:
        case apigen_typeid_char:
        case apigen_typeid_u8:
        case apigen_typeid_u16:
        case apigen_typeid_u32:
        case apigen_typeid_u64:
        case apigen_typeid_usize:
        case apigen_typeid_c_ushort:
        case apigen_typeid_c_uint:
        case apigen_typeid_c_ulong:
        case apigen_typeid_c_ulonglong:
        case apigen_typeid_i8:
        case apigen_typeid_i16:
        case apigen_typeid_i32:
        case apigen_typeid_i64:
        case apigen_typeid_isize:
        case apigen_typeid_c_short:
        case apigen_typeid_c_int:
        case apigen_typeid_c_long:
        case apigen_typeid_c_longlong:
            return true;
        default: 
            return false;
    }
}

struct ValueRange { int64_t min; uint64_t max; };

static bool range_is_valid(struct ValueRange range)
{
    if(range.min < 0) {
        return true;
    }
    return (range.max > (uint64_t)range.min);
}

static bool svalue_in_range(struct ValueRange range, int64_t ival)
{
    return (ival >= range.min) && ((ival < 0) || ((uint64_t)ival <= range.max));
}

static bool uvalue_in_range(struct ValueRange range, uint64_t uval)
{
    return (uval <= range.max) && ((range.min < 0) || (uval >= (uint64_t)range.min));
}

static const struct ValueRange INIT_LIMIT_RANGE = { .min = INT64_MAX, .max = 0 };

static void insert_ival_into_range(struct ValueRange * range, int64_t ival)
{
    APIGEN_NOT_NULL(range);
    if(range->min > ival) range->min = ival;
    if((ival > 0) && range->max < (uint64_t)ival) range->max = (uint64_t)ival;
}

static void insert_uval_into_range(struct ValueRange * range, uint64_t uval)
{
    APIGEN_NOT_NULL(range);
    
    if((range->min > 0) && ((uint64_t)range->min > uval)) range->min = (int64_t)uval;
    if(range->max < uval) range->max = uval;
}

static struct ValueRange get_integer_range(enum apigen_TypeId type)
{
    switch(type) {
        case apigen_typeid_uchar:       return (struct ValueRange){ .min =                    0LL, .max =               0xFFULL };
        case apigen_typeid_ichar:       return (struct ValueRange){ .min =                 -128LL, .max =               0x7FULL };
        case apigen_typeid_char:        return (struct ValueRange){ .min =                    0LL, .max =               0x7FULL };

        case apigen_typeid_u8:          return (struct ValueRange){ .min =          0, .max = UINT8_MAX };
        case apigen_typeid_u16:         return (struct ValueRange){ .min =          0, .max = UINT16_MAX };
        case apigen_typeid_u32:         return (struct ValueRange){ .min =          0, .max = UINT32_MAX };
        case apigen_typeid_u64:         return (struct ValueRange){ .min =          0, .max = UINT64_MAX };
        
        case apigen_typeid_i8:          return (struct ValueRange){ .min = INT8_MIN,   .max = INT8_MAX };
        case apigen_typeid_i16:         return (struct ValueRange){ .min = INT16_MIN,  .max = INT16_MAX };
        case apigen_typeid_i32:         return (struct ValueRange){ .min = INT32_MIN,  .max = INT32_MAX };
        case apigen_typeid_i64:         return (struct ValueRange){ .min = INT64_MIN,  .max = INT64_MAX };

        // architecture dependent types:

        case apigen_typeid_usize:       return (struct ValueRange){ .min = 0, .max = 0 };
        case apigen_typeid_isize:       return (struct ValueRange){ .min = 0, .max = 0 };

        // compiler+platform dependent types:

        case apigen_typeid_c_ushort:    return (struct ValueRange){ .min = 0, .max = 0 };
        case apigen_typeid_c_uint:      return (struct ValueRange){ .min = 0, .max = 0 };
        case apigen_typeid_c_ulong:     return (struct ValueRange){ .min = 0, .max = 0 };
        case apigen_typeid_c_ulonglong: return (struct ValueRange){ .min = 0, .max = 0 };

        case apigen_typeid_c_short:     return (struct ValueRange){ .min = 0, .max = 0 };
        case apigen_typeid_c_int:       return (struct ValueRange){ .min = 0, .max = 0 };
        case apigen_typeid_c_long:      return (struct ValueRange){ .min = 0, .max = 0 };
        case apigen_typeid_c_longlong:  return (struct ValueRange){ .min = 0, .max = 0 };

        default:                        APIGEN_UNREACHABLE();
    }
}

static bool is_integer_unsigned(enum apigen_TypeId type)
{
    switch(type) {
        case apigen_typeid_uchar:       return true;
        case apigen_typeid_ichar:       return false;
        case apigen_typeid_char:        return true; // not quite right, but good enough (we only allow 0...127)

        case apigen_typeid_u8:          return true;
        case apigen_typeid_u16:         return true;
        case apigen_typeid_u32:         return true;
        case apigen_typeid_u64:         return true;
        
        case apigen_typeid_i8:          return false;
        case apigen_typeid_i16:         return false;
        case apigen_typeid_i32:         return false;
        case apigen_typeid_i64:         return false;

        // architecture dependent types:

        case apigen_typeid_usize:       return true;
        case apigen_typeid_isize:       return false;

        // compiler+platform dependent types:

        case apigen_typeid_c_ushort:    return true;
        case apigen_typeid_c_uint:      return true;
        case apigen_typeid_c_ulong:     return true;
        case apigen_typeid_c_ulonglong: return true;

        case apigen_typeid_c_short:     return false;
        case apigen_typeid_c_int:       return false;
        case apigen_typeid_c_long:      return false;
        case apigen_typeid_c_longlong:  return false;

        default:                        APIGEN_UNREACHABLE();
    }
}

static enum apigen_TypeId mapPtrType(enum apigen_ParserTypeId src_type_id, bool is_const, bool is_optional)
{
    switch(src_type_id) {

        case apigen_parser_type_ptr_to_one:
            return is_const
                ? (is_optional ? apigen_typeid_nullable_const_ptr_to_one : apigen_typeid_const_ptr_to_one) // const 
                : (is_optional ? apigen_typeid_nullable_ptr_to_one       : apigen_typeid_ptr_to_one)       // mut
            ;

        case apigen_parser_type_ptr_to_many:
            return is_const
                ? (is_optional ? apigen_typeid_nullable_const_ptr_to_many : apigen_typeid_const_ptr_to_many) // const 
                : (is_optional ? apigen_typeid_nullable_ptr_to_many       : apigen_typeid_ptr_to_many)       // mut
            ;

        case apigen_parser_type_ptr_to_many_sentinelled:
            return is_const
                ? (is_optional ? apigen_typeid_nullable_const_ptr_to_sentinelled_many : apigen_typeid_const_ptr_to_sentinelled_many) // const 
                : (is_optional ? apigen_typeid_nullable_ptr_to_sentinelled_many       : apigen_typeid_ptr_to_sentinelled_many)       // mut
            ;

        default: apigen_panic("Unmapped type");
    }
}

enum ResolveStateResponse
{
    RESOLVE_FAILED_GENERIC = 3,
    RESOLVE_MISSING_SYMBOL = 10,
};

struct ResolveState 
{
    struct apigen_ParserState * parser;
    struct apigen_TypePool * pool;

    bool emit_resolve_errors;    
    jmp_buf generic_error_retpoint;
};

static void APIGEN_NORETURN handle_resolve_error(struct ResolveState * state, struct apigen_ParserType const * const type)
{
    APIGEN_NOT_NULL(state);
    APIGEN_NOT_NULL(type);
    APIGEN_ASSERT(type->type == apigen_parser_type_named);
    if(state->emit_resolve_errors) {
        emit_diagnostics(state->parser, type->location, apigen_error_undeclared_identifier, type->named_data);
    }
    longjmp(state->generic_error_retpoint, RESOLVE_MISSING_SYMBOL);
}

static void APIGEN_NORETURN exit_type_resolution(struct ResolveState * state)
{
    APIGEN_NOT_NULL(state);
    longjmp(state->generic_error_retpoint, RESOLVE_FAILED_GENERIC);
}

static struct apigen_Type const * resolve_type_inner(struct ResolveState * resolver, struct apigen_ParserType const * src_type)
{
    APIGEN_NOT_NULL(resolver);
    APIGEN_NOT_NULL(src_type);

    switch(src_type->type) {

        case apigen_parser_type_named: {
            struct apigen_Type const * const named_type = apigen_lookup_type(resolver->pool, src_type->named_data);
            if(named_type == NULL) {
                handle_resolve_error(resolver, src_type);
            }
            return named_type;
        }
            
        case apigen_parser_type_ptr_to_one:
        case apigen_parser_type_ptr_to_many:
        case apigen_parser_type_ptr_to_many_sentinelled: {
            struct apigen_Type const * const underlying_type = resolve_type_inner(resolver, src_type->pointer_data.underlying_type);
            if(underlying_type == NULL) {
                return NULL;
            }
            
            struct apigen_Pointer const extra_data = (struct apigen_Pointer) {
                .underlying_type = underlying_type,
                .sentinel        = src_type->pointer_data.sentinel,
            };
            
            struct apigen_Type const pointer_type = {
                .name = NULL,
                .id = mapPtrType(src_type->type, src_type->pointer_data.is_const, src_type->pointer_data.is_optional),
                .extra = &extra_data,
            };

            return apigen_intern_type(resolver->pool, &pointer_type);
        }

        case apigen_parser_type_array: {
            struct apigen_Type const * const underlying_type = resolve_type_inner(resolver, src_type->array_data.underlying_type);
            if(underlying_type == NULL) {
                return NULL;
            }

            if(src_type->array_data.size.type != apigen_value_uint) {
                emit_diagnostics(
                    resolver->parser,
                    src_type->location,
                    apigen_error_array_size_not_uint
                );
                exit_type_resolution(resolver);
            }

            struct apigen_Array const extra_data = (struct apigen_Array) {
                .underlying_type = underlying_type,
                .size            = src_type->array_data.size.value_uint,
            };
            
            struct apigen_Type const array_type = {
                .name = NULL,
                .id = apigen_typeid_array,
                .extra = &extra_data,
            };

            return apigen_intern_type(resolver->pool, &array_type);
        }

        case apigen_parser_type_function: {
            struct apigen_Type const * const return_type = resolve_type_inner(resolver, src_type->function_data.return_type);
            if(return_type == NULL) {
                return NULL;
            }

            size_t parameter_count = 0;
            {
                struct apigen_ParserField const * param_iter = src_type->function_data.parameters;
                while(param_iter != NULL) {
                    parameter_count += 1;
                    param_iter = param_iter->next;
                }
            }

            struct apigen_NamedValue * const parameters = apigen_memory_arena_alloc(resolver->pool->arena, parameter_count * sizeof(struct apigen_Type));
            {
                size_t index = 0;
                struct apigen_ParserField const * param_iter = src_type->function_data.parameters;
                while(param_iter != NULL) {

                    for(size_t i = 0; i < index; i++) {
                        if(apigen_streq(parameters[i].name, param_iter->identifier)) {
                            emit_diagnostics(resolver->parser, param_iter->location, apigen_error_duplicate_parameter, param_iter->identifier);
                            exit_type_resolution(resolver);
                        }
                    }

                    parameters[index] = (struct apigen_NamedValue) {
                        .documentation = param_iter->documentation,
                        .name = param_iter->identifier,
                        .type = resolve_type_inner(resolver, &param_iter->type),
                    };
                    if(parameters[index].type == NULL) {
                        return NULL;
                    }

                    index += 1;
                    param_iter = param_iter->next;
                }
                APIGEN_ASSERT(parameter_count == index);
            }

            struct apigen_Function const extra_data = (struct apigen_Function) {
                .return_type = return_type,
                .parameter_count = parameter_count,
                .parameters = parameters,  
            };
            
            struct apigen_Type const array_type = {
                .name = NULL,
                .id = apigen_typeid_function,
                .extra = &extra_data,
            };

            return apigen_intern_type(resolver->pool, &array_type);
        }

        case apigen_parser_type_enum: 
        case apigen_parser_type_struct:
        case apigen_parser_type_union: 
        case apigen_parser_type_opaque: { 
            apigen_panic("cannot resolve unique type in this part of the code!");
        }
    }

    return NULL;
}

static struct apigen_Type const * resolve_type(
        struct apigen_ParserState * const parser,
        struct apigen_TypePool * const pool,
        bool emit_resolve_errors,
        struct apigen_ParserType const * src_type,
        bool * non_resolve_error // will be set to `true` if a non-resolution error occurred
    )
{
    APIGEN_NOT_NULL(parser);
    APIGEN_NOT_NULL(pool);
    APIGEN_NOT_NULL(src_type);

    struct ResolveState resolve_state = {
        .parser = parser,
        .pool = pool,
        .emit_resolve_errors = emit_resolve_errors,
    };

    int const response = setjmp(resolve_state.generic_error_retpoint);
    if(response == 0)
    {
        struct apigen_Type const * const resolved_type = resolve_type_inner(&resolve_state, src_type);
        APIGEN_NOT_NULL(resolved_type);
        return resolved_type;
    }
    else if(response == RESOLVE_FAILED_GENERIC)
    {
        // TODO: Handle arbitrary error
        if(non_resolve_error) *non_resolve_error = true;
        return NULL;
    }
    else if(response == RESOLVE_MISSING_SYMBOL)
    {
        // TODO: Handle resolution error
        return NULL;
    }
    else 
    {
        apigen_panic("Invalid response from setjmp, please validate your code!");
    }
}

static enum apigen_TypeId map_unique_parser_type_id(enum apigen_ParserTypeId id) {
    APIGEN_ASSERT(is_unique_type(id));
    switch(id) {
        case apigen_parser_type_enum:                       return apigen_typeid_enum;
        case apigen_parser_type_struct:                     return apigen_typeid_struct;
        case apigen_parser_type_union:                      return apigen_typeid_union;
        case apigen_parser_type_opaque:                     return apigen_typeid_opaque;
        case apigen_parser_type_named:                      APIGEN_UNREACHABLE();
        case apigen_parser_type_array:                      APIGEN_UNREACHABLE();
        case apigen_parser_type_ptr_to_one:                 APIGEN_UNREACHABLE();
        case apigen_parser_type_ptr_to_many:                APIGEN_UNREACHABLE();
        case apigen_parser_type_ptr_to_many_sentinelled:    APIGEN_UNREACHABLE();
        case apigen_parser_type_function:                   APIGEN_UNREACHABLE();
    }
}

bool apigen_analyze(struct apigen_ParserState * const state, struct apigen_Document * const out_document)
{
    APIGEN_NOT_NULL(state);
    APIGEN_NOT_NULL(out_document);

    APIGEN_ASSERT(state->top_level_declarations != NULL);

    *out_document = (struct apigen_Document) {
        .type_pool = {
            .arena = state->ast_arena,
        },

        .type_count     = 0,
        .types          = NULL,

        .function_count = 0,
        .functions      = NULL,

        .variable_count = 0,
        .variables      = NULL,

        .constant_count = 0,
        .constants      = NULL,
    };

    // Phase 1: Figure out how much memory we need for all exported declarations:
    {
        struct apigen_ParserDeclaration const * decl = state->top_level_declarations;
        while(decl != NULL) {
            switch(decl->kind) {
                case apigen_parser_const_declaration:
                case apigen_parser_var_declaration:
                    out_document->variable_count += 1;
                    break;

                case apigen_parser_constexpr_declaration:
                    out_document->constant_count += 1;
                    break;

                case apigen_parser_fn_declaration:
                    out_document->function_count += 1;
                    break;

                case apigen_parser_type_declaration:
                    out_document->type_count += 1;
                    break;
            }
            decl = decl->next;
        }

        out_document->types     = apigen_memory_arena_alloc(state->ast_arena, out_document->type_count     * sizeof(struct apigen_Type const *));
        out_document->functions = apigen_memory_arena_alloc(state->ast_arena, out_document->function_count * sizeof(struct apigen_Type const *));
        out_document->variables = apigen_memory_arena_alloc(state->ast_arena, out_document->variable_count * sizeof(struct apigen_Global));
        out_document->constants = apigen_memory_arena_alloc(state->ast_arena, out_document->constant_count * sizeof(struct apigen_Constant));
    }

    // Phase 2: Publish all named unique types (struct, union, ...) into the pool
    {
        bool ok = true;

        struct apigen_ParserDeclaration * decl = state->top_level_declarations;
        while(decl != NULL) {
            if(decl->kind == apigen_parser_type_declaration) {
                if(is_unique_type( decl->type.type)) {
                    struct apigen_Type * const unique_type = apigen_memory_arena_alloc(state->ast_arena, sizeof(struct apigen_Type));
                    *unique_type = (struct apigen_Type) {
                        .name = decl->identifier,
                        .id = map_unique_parser_type_id(decl->type.type),
                        .extra = NULL, // no internal resolution yet
                    };

                    if(!apigen_register_type(&out_document->type_pool, unique_type, NULL)) {
                        emit_diagnostics(state, decl->location, apigen_error_duplicate_symbol, decl->identifier);
                        ok = false;
                    }

                    decl->associated_type = unique_type;
                }
            }
            decl = decl->next;
        }
        if(!ok) {
            return false;
        }
    }

    // Phase 3: Resolve and publish all global non-unique types (pointers, aliases, ...)
    {
        size_t resolved_count;
        size_t resolve_failed_count;
        bool non_resolve_error;
        bool emit_resolve_errors = false;
        while(true) {
            resolved_count = 0;
            resolve_failed_count = 0;
            non_resolve_error = false;

            struct apigen_ParserDeclaration * decl = state->top_level_declarations;
            while(decl != NULL) {
                if(decl->kind == apigen_parser_type_declaration) {
                    if(decl->associated_type == NULL) {
                        APIGEN_ASSERT(!is_unique_type( decl->type.type));
                    
                        struct apigen_Type const * const resolved_type = resolve_type(state, &out_document->type_pool, emit_resolve_errors, &decl->type, &non_resolve_error);
                        if(resolved_type != NULL) {

                            if(apigen_register_type(&out_document->type_pool, resolved_type, decl->identifier)) {
                                decl->associated_type = (struct apigen_Type *)resolved_type; // This is fine, we don't need the mutable reference here anyways
                                resolved_count += 1;
                            }
                            else {
                                emit_diagnostics(state, decl->location, apigen_error_duplicate_symbol, decl->identifier);
                                non_resolve_error = true;
                            }

                        }
                        else {
                            resolve_failed_count += 1;
                        }
                    }
                }
                decl = decl->next;
            }

            if(resolved_count == 0) {
                if(emit_resolve_errors) {
                    break;
                }
                emit_resolve_errors = true;
            }
            if(non_resolve_error) {
                break;
            }
        };

        if(non_resolve_error) {
            return false;
        }

        if(resolve_failed_count > 0) {
            emit_diagnostics(state, (struct apigen_ParserLocation){}, apigen_error_unresolved_symbols, resolve_failed_count);
            return false;
        }
    }

    // Phase 4: Now resolve all unique types
    {
        bool ok = true;
        struct apigen_ParserDeclaration const * decl = state->top_level_declarations;
        while(decl != NULL) {
            if(decl->kind == apigen_parser_type_declaration) {
                if(is_unique_type(decl->type.type)) {
                    APIGEN_ASSERT(decl->associated_type != NULL);
                    switch(decl->type.type)
                    {
                        case apigen_parser_type_struct:
                        case apigen_parser_type_union:
                        {
                            apigen_panic("implement analyzing struct/union");
                        }

                        case apigen_parser_type_enum:
                        {
                            struct ValueRange int_range = { 0, 0 };

                            struct apigen_Type const * underlying_type = NULL;
                            if(decl->type.enum_data.underlying_type != NULL) {
                                underlying_type = resolve_type(state, &out_document->type_pool, true, decl->type.enum_data.underlying_type, NULL);
                                if(underlying_type != NULL) {
                                    if(is_integer_type(*underlying_type)) {
                                        int_range = get_integer_range(underlying_type->id);

                                        if(!range_is_valid(int_range)) {
                                            emit_diagnostics(state, decl->type.enum_data.underlying_type->location, apigen_warning_enum_int_undefined, "u32");
                                        }
                                    }
                                    else {
                                        emit_diagnostics(state, decl->type.enum_data.underlying_type->location, apigen_error_enum_type_must_be_int);
                                        ok = false;
                                    }
                                }
                                else {
                                    // Has already emit an error here
                                    ok = false;
                                }
                            }

                            size_t items_count = 0;
                            {
                                struct apigen_ParserEnumItem const * iter = decl->type.enum_data.items;
                                while(iter != NULL) {
                                    items_count += 1;
                                    iter = iter->next;
                                }
                            }

                            if(items_count > 0)
                            {
                                struct apigen_EnumItem * const items = apigen_memory_arena_alloc(out_document->type_pool.arena, items_count * sizeof(struct apigen_EnumItem));
                                {
                                    size_t index = 0;
                                    struct apigen_ParserEnumItem const * iter = decl->type.enum_data.items;
                                    union {
                                        uint64_t uval;
                                        int64_t ival;
                                    } current_value = { .uval = 0 };

                                    bool value_is_signed = (underlying_type != NULL) ? is_integer_unsigned(underlying_type->id) : false;

                                    struct ValueRange actual_range = INIT_LIMIT_RANGE;
                                    
                                    while(iter != NULL) {

                                        for(size_t i = 0; i < index; i++)
                                        {
                                            if(apigen_streq(items[i].name, iter->identifier)) {
                                                emit_diagnostics(state, iter->location, apigen_error_duplicate_enum_item, iter->identifier);
                                                break;
                                            }
                                        }

                                        bool skip_range_check = false;
                                        switch(iter->value.type) {
                                            case apigen_value_null: break; // null values don't change the current value
                                            
                                            case apigen_value_str:
                                                emit_diagnostics(state, iter->location, apigen_error_enum_value_illegal, iter->identifier);
                                                break;
                                            
                                            case apigen_value_sint:
                                                // fprintf(stderr, "ival=%" PRIi64 "\n", iter->value.value_sint);
                                                // NOTE: all signed ints are less than zero!

                                                if((underlying_type != NULL) && !value_is_signed) {
                                                    char buffer[256];
                                                    (void)snprintf(buffer, sizeof buffer, "%"PRId64, iter->value.value_sint);
                                                    emit_diagnostics(state, iter->location, apigen_error_enum_out_of_range, buffer, iter->identifier);
                                                    skip_range_check = true;
                                                }
                                                else if(value_is_signed) {
                                                    current_value.ival = iter->value.value_sint;
                                                }
                                                else {
                                                    value_is_signed = true;
                                                    current_value.ival = iter->value.value_sint;
                                                }

                                                break;

                                            case apigen_value_uint:
                                                // fprintf(stderr, "uval=%" PRIu64 "\n", iter->value.value_uint);
                                                if(value_is_signed) {
                                                    if(iter->value.value_uint > INT64_MAX) {
                                                        char buffer[256];
                                                        (void)snprintf(buffer, sizeof buffer, "%"PRId64, iter->value.value_uint);
                                                        emit_diagnostics(state, iter->location, apigen_error_enum_out_of_range, buffer, iter->identifier);
                                                        skip_range_check = true;
                                                    }
                                                    else {
                                                        current_value.ival = (int64_t)iter->value.value_uint;
                                                    }
                                                }
                                                else {
                                                    
                                                    current_value.uval = iter->value.value_uint;
                                                }
                                                break;
                                        }

                                        if(!skip_range_check && range_is_valid(int_range)) {
                                            if(value_is_signed) {
                                                if(!svalue_in_range(int_range, current_value.ival)) {
                                                    char buffer[256];
                                                    (void)snprintf(buffer, sizeof buffer, "%"PRId64, current_value.ival);
                                                    emit_diagnostics(state, iter->location, apigen_error_enum_out_of_range, buffer, iter->identifier);
                                                }
                                            }
                                            else {
                                                if(!uvalue_in_range(int_range, current_value.uval)) {
                                                    char buffer[256];
                                                    (void)snprintf(buffer, sizeof buffer, "%"PRIu64, current_value.uval);
                                                    emit_diagnostics(state, iter->location, apigen_error_enum_out_of_range, buffer, iter->identifier);
                                                }
                                            }
                                        }

                                        for(size_t i = 0; i < index; i++) 
                                        {
                                            // we can safely compare uval as we're comparing for "bit pattern equality"
                                            if(items[i].uvalue == current_value.uval) { 
                                                char buffer[256];
                                                if(value_is_signed) {
                                                    (void)snprintf(buffer, sizeof buffer, "%"PRId64, current_value.ival);
                                                } else {
                                                    (void)snprintf(buffer, sizeof buffer, "%"PRIu64, current_value.uval);
                                                }
                                                emit_diagnostics(state, iter->location, apigen_error_duplicate_enum_value, iter->identifier, buffer, items[i].name);
                                                break;
                                            }
                                        }   
                                        
                                        struct apigen_EnumItem * const item = &items[index];
                                        if(value_is_signed) {
                                            *item = (struct apigen_EnumItem) {
                                                .documentation = iter->documentation,
                                                .name          = iter->identifier,
                                                .ivalue        = current_value.ival,
                                            };
                                            insert_ival_into_range(&actual_range, current_value.ival);
                                            current_value.ival += 1;
                                        }
                                        else {
                                            *item = (struct apigen_EnumItem) {
                                                .documentation = iter->documentation,
                                                .name          = iter->identifier,
                                                .uvalue        = current_value.uval,
                                            };
                                            insert_uval_into_range(&actual_range, current_value.uval);
                                            current_value.uval += 1;

                                        }
                                        iter = iter->next;
                                        index += 1;
                                    }
                                    APIGEN_ASSERT(index == items_count);

                                    if(underlying_type == NULL)
                                    {
                                        // auto-detect value type
                                        if(actual_range.min < 0) {
                                            bool const fits_i8 = (actual_range.min >= INT8_MIN) && (actual_range.max <= INT8_MAX);
                                            bool const fits_i16 = (actual_range.min >= INT16_MIN) && (actual_range.max <= INT16_MAX);
                                            bool const fits_i32 = (actual_range.min >= INT32_MIN) && (actual_range.max <= INT32_MAX);
                                            
                                            underlying_type = fits_i8  ? apigen_get_builtin_type(apigen_typeid_i8)
                                                            : fits_i16 ? apigen_get_builtin_type(apigen_typeid_i16)
                                                            : fits_i32 ? apigen_get_builtin_type(apigen_typeid_i32)
                                                            :            apigen_get_builtin_type(apigen_typeid_i64);
                                        }
                                        else {
                                            bool const fits_u8 = (actual_range.max <= UINT8_MAX);
                                            bool const fits_u16 = (actual_range.max <= UINT16_MAX);
                                            bool const fits_u32 = (actual_range.max <= UINT32_MAX);
                                            
                                            underlying_type = fits_u8  ? apigen_get_builtin_type(apigen_typeid_u8)
                                                            : fits_u16 ? apigen_get_builtin_type(apigen_typeid_u16)
                                                            : fits_u32 ? apigen_get_builtin_type(apigen_typeid_u32)
                                                            :            apigen_get_builtin_type(apigen_typeid_u64);
                                        }
                                    }
                                }

                                APIGEN_ASSERT(underlying_type != NULL);

                                // {
                                //     fprintf(stderr, "type: %s\n", apigen_type_str(underlying_type->id));
                                //     bool const value_is_signed = !is_integer_unsigned(underlying_type->id);
                                //     for(size_t i = 0; i < items_count; i++)
                                //     {
                                //         if(value_is_signed) {
                                //             fprintf(stderr, "%zu: %s => %"PRIi64"\n", i, items[i].name, items[i].ivalue);
                                //         }
                                //         else {
                                //             fprintf(stderr, "%zu: %s => %"PRIu64"\n", i, items[i].name, items[i].uvalue);
                                //         }
                                //     }
                                // }

                                struct apigen_Enum * enum_extra = apigen_memory_arena_alloc(out_document->type_pool.arena, sizeof(struct apigen_Enum));
                                *enum_extra = (struct apigen_Enum) {
                                    .underlying_type = underlying_type,
                                    .item_count = items_count,
                                    .items = items,
                                };
                                decl->associated_type->extra = enum_extra;
                            }
                            else 
                            {
                                emit_diagnostics(state, decl->type.location, apigen_error_enum_empty);
                                ok = false;
                            }
                            
                            // struct apigen_ParserEnumItem * items;
                            // apigen_panic("implement analyzing enums");
                        }

                        case apigen_parser_type_opaque:
                            // opaque types do not have any EXTRA, they are already fully resolved.
                            break;

                        default:
                            APIGEN_UNREACHABLE();
                    }
                }                
            }
            decl = decl->next;
        }
        if(!ok) {
            return false;
        }
    }

    // Phase 5: Store all declared type into the document
    {
        size_t index = 0;
        struct apigen_ParserDeclaration const * decl = state->top_level_declarations;
        while(decl != NULL) {
            if(decl->kind == apigen_parser_type_declaration) {
                APIGEN_ASSERT(decl->associated_type != NULL);
                out_document->types[index] = decl->associated_type;
                index += 1;
            }
            decl = decl->next;
        }
        APIGEN_ASSERT(index == out_document->type_count);
    }

    return true;
}
