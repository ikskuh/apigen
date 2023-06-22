#include "apigen.h"

#include <string.h>
#include <inttypes.h>
#include <ctype.h>

enum RenderMode {
  TYPE_REFERENCE,
  TYPE_INSTANCE,
};

static struct apigen_Type const * unalias(struct apigen_Type const * type)
{
    while(type->id == apigen_typeid_alias) {
        type = type->extra;
    }
    return type;
}


static void render_value(struct apigen_Stream const stream, struct apigen_Value value)
{
  switch(value.type) {
    case apigen_value_null: apigen_io_write(stream, "NULL", 4); break;
    case apigen_value_sint: apigen_io_printf(stream, "%"PRIi64, value.value_sint); break;
    case apigen_value_uint: apigen_io_printf(stream, "%"PRIu64, value.value_uint); break;
    case apigen_value_str: apigen_io_printf(stream, "\"%s\"", value.value_str); break;
  }
} 

static void flush_indent(struct apigen_Stream const stream, size_t indent)
{
  for(size_t i = 0; i < indent; i++) {
    apigen_io_write(stream, "    ", 4);
  }
}

static void render_docstring(struct apigen_Stream const stream, size_t indent, char const * docstring)
{
  while(true)
  {
    size_t l = 0;
    bool lf = false;
    for(l = 0; docstring[l]; l++) {
      if(docstring[l] == '\n') {
        lf = true;
        break;
      }
    }

    flush_indent(stream, indent);
    apigen_io_write(stream, "/// ", 4);
    apigen_io_write(stream, docstring, l);
    apigen_io_write(stream, "\n", 1);

    if(!lf) {
      break;
    }

    docstring += (l + 1);
  }
}

enum IdentifierTransform
{
    ID_KEEP,
    ID_UPPERCASE,
    ID_LOWERCASE,
};

static size_t len_min(size_t a, size_t b) {
    return (a < b) ? a : b;
}

static void render_identifier(struct apigen_Stream const stream, enum IdentifierTransform transform, char const * identifier, bool exact_match)
{
    static char const * const reserved_identifiers[] = {
        // true keywords
        "alignas",    "alignof",   "auto",           "bool",          "break",      "case",
        "char",       "const",     "constexpr",      "continue",      "default",    "do",
        "double",     "else",      "enum",           "extern",        "false",      "float",
        "for",        "goto",      "if",             "inline",        "int",        "long",
        "nullptr",    "register",  "restrict",       "return",        "short",      "signed",
        "sizeof",     "static",    "static_assert",  "struct",        "switch",     "thread_local",
        "true",       "typedef",   "typeof",         "typeof_unqual", "union",      "unsigned",
        "volatile",   "while",     "_Alignas",       "_Alignof",      "_Atomic",    "_BitInt",
        "_Bool",      "_Complex",  "_Decimal128",    "_Decimal32",    "_Decimal64", "_Generic ",
        "_Imaginary", "_Noreturn", "_Static_assert", "_Thread_local", "void",

        // typical aliases:
        "alignas",   "alignof",  "bool",          "complex",
        "imaginary", "noreturn", "static_assert", "thread_local",

        NULL,   
    };

    bool reserved = false;
    if((strlen(identifier) > 2) && (identifier[0] == '_') && ((identifier[1] == '_') || ((identifier[1] >= 'A') && (identifier[1] <= 'Z')))) {
        reserved = true;
    }

    for(size_t i = 0; !reserved && reserved_identifiers[i]; i++) {
        if(apigen_streq(identifier, reserved_identifiers[i])) {
            reserved = true;
        }
    }

    switch(transform) {
        case ID_KEEP:
            apigen_io_printf(stream, "%s", identifier);
            break;
        case ID_LOWERCASE:
        case ID_UPPERCASE: {
            char chunk[16];
            while(*identifier) {
                size_t len = len_min(sizeof(chunk), strlen(identifier));

                for(size_t i = 0; i < len; i++) {
                    chunk[i] = (transform == ID_LOWERCASE) ? tolower(identifier[i]) : toupper(identifier[i]);
                }
                apigen_io_write(stream, chunk, len);

                identifier += len;
            }

            break;
        }
    }

    if(reserved) {
        if(exact_match) {
            apigen_panic("used unrecoverable reserved identifier!");
        }
        apigen_io_printf(stream, "_", identifier);
    }
}

// static void render_value(struct apigen_Stream const stream, struct apigen_Value value)
// {
//   switch(value.type) {
//     case apigen_value_null: apigen_io_write(stream, "null", 4); break;
//     case apigen_value_sint: apigen_io_printf(stream, "%"PRIi64, value.value_sint); break;
//     case apigen_value_uint: apigen_io_printf(stream, "%"PRIu64, value.value_uint); break;
//     case apigen_value_str: apigen_io_printf(stream, "\"%s\"", value.value_str); break;
//   }
// } 

enum DeclarationKind {
    DECL_REGULAR = 0,
    DECL_CONST = 1,
};

static void render_declaration(struct apigen_Stream const stream, enum DeclarationKind kind, char const * identifier, enum IdentifierTransform transform, struct apigen_Type const * const type, enum RenderMode render_mode, size_t indent);

static void render_type_prefix(struct apigen_Stream const stream, struct apigen_Type const * const type, enum RenderMode render_mode, size_t indent);
static void render_type_suffix(struct apigen_Stream const stream, struct apigen_Type const * const type, enum RenderMode render_mode, size_t indent);

static void render_parameter_list(struct apigen_Stream const stream, struct apigen_FunctionType func, size_t indent)
{
  apigen_io_printf(stream, "(\n"); 
  for(size_t i = 0; i < func.parameter_count; i++)
  {
    struct apigen_NamedValue const param = func.parameters[i];

    if(param.documentation != NULL) {
      render_docstring(stream, indent + 1, param.documentation);
    }
    flush_indent(stream, indent + 1);

    render_declaration(stream, DECL_REGULAR, param.name, ID_LOWERCASE, param.type, TYPE_REFERENCE, indent + 1);

    if(i + 1 == func.parameter_count) {
      apigen_io_printf(stream, "\n");
    }
    else {
      apigen_io_printf(stream, ",\n");
    }
  }
  flush_indent(stream, indent);
  apigen_io_printf(stream, ") "); 
}


static void render_type_prefix(struct apigen_Stream const stream, struct apigen_Type const * const type, enum RenderMode render_mode, size_t indent)
{
    if((render_mode == TYPE_REFERENCE) && type->name) {

        switch(type->id) {
            case apigen_typeid_struct: apigen_io_write(stream, "struct ", 7); break;
            case apigen_typeid_union:  apigen_io_write(stream, "union ", 6); break;
            default: break;
        }

        render_identifier(stream, ID_KEEP, type->name, false); 
        return;
    }

    switch(type->id)
    {
        struct apigen_Pointer const * pointer;
        struct apigen_Array const * array;
        struct apigen_FunctionType const * func;
        struct apigen_Enum const * enumeration;
        struct apigen_UnionOrStruct const * uos;

        case apigen_typeid_void:        apigen_io_printf(stream, "void"); break;
        case apigen_typeid_anyopaque:   apigen_io_printf(stream, "void"); break;
        case apigen_typeid_opaque:      apigen_io_printf(stream, "void"); break; // opaque types can be declared void and will get named void-pointers then
        case apigen_typeid_bool:        apigen_io_printf(stream, "bool"); break;
        case apigen_typeid_uchar:       apigen_io_printf(stream, "unsigned char"); break;
        case apigen_typeid_ichar:       apigen_io_printf(stream, "signed char"); break;
        case apigen_typeid_char:        apigen_io_printf(stream, "char"); break;

        case apigen_typeid_u8:          apigen_io_printf(stream, "uint8_t"); break;
        case apigen_typeid_u16:         apigen_io_printf(stream, "uint16_t"); break;
        case apigen_typeid_u32:         apigen_io_printf(stream, "uint32_t"); break;
        case apigen_typeid_u64:         apigen_io_printf(stream, "uint64_t"); break;
        case apigen_typeid_usize:       apigen_io_printf(stream, "uintptr_t"); break;
        case apigen_typeid_c_ushort:    apigen_io_printf(stream, "unsigned short"); break;
        case apigen_typeid_c_uint:      apigen_io_printf(stream, "unsigned int"); break;
        case apigen_typeid_c_ulong:     apigen_io_printf(stream, "unsigned long"); break;
        case apigen_typeid_c_ulonglong: apigen_io_printf(stream, "unsigned long long"); break;

        case apigen_typeid_i8:          apigen_io_printf(stream, "int8_t"); break;
        case apigen_typeid_i16:         apigen_io_printf(stream, "int16_t"); break;
        case apigen_typeid_i32:         apigen_io_printf(stream, "int32_t"); break;
        case apigen_typeid_i64:         apigen_io_printf(stream, "int64_t"); break;
        case apigen_typeid_isize:       apigen_io_printf(stream, "intptr_t"); break;
        case apigen_typeid_c_short:     apigen_io_printf(stream, "short"); break;
        case apigen_typeid_c_int:       apigen_io_printf(stream, "int"); break;
        case apigen_typeid_c_long:      apigen_io_printf(stream, "long"); break;
        case apigen_typeid_c_longlong:  apigen_io_printf(stream, "long long"); break;

        case apigen_typeid_f32:         apigen_io_printf(stream, "float"); break;
        case apigen_typeid_f64:         apigen_io_printf(stream, "double"); break;

        case apigen_typeid_ptr_to_one:
        case apigen_typeid_ptr_to_many:
        case apigen_typeid_ptr_to_sentinelled_many:
        case apigen_typeid_nullable_ptr_to_one:
        case apigen_typeid_nullable_ptr_to_many:
        case apigen_typeid_nullable_ptr_to_sentinelled_many:
            pointer = type->extra;

            render_type_prefix(stream, pointer->underlying_type, TYPE_REFERENCE, indent);
            apigen_io_printf(stream, " *");
            break;

        case apigen_typeid_const_ptr_to_one:
        case apigen_typeid_const_ptr_to_many:
        case apigen_typeid_const_ptr_to_sentinelled_many:
        case apigen_typeid_nullable_const_ptr_to_one:
        case apigen_typeid_nullable_const_ptr_to_many:
        case apigen_typeid_nullable_const_ptr_to_sentinelled_many:
            pointer = type->extra;
            render_type_prefix(stream, pointer->underlying_type, TYPE_REFERENCE, indent);
            if(unalias(pointer->underlying_type)->id == apigen_typeid_function) {
                // special casing for function pointers: 
                // In C, a const function pointer cannot exist
                apigen_io_printf(stream, " *");
            }
            else {
                apigen_io_printf(stream, " const *");
            }
            break;


        case apigen_typeid_array:
            array = type->extra;
            render_type_prefix(stream, array->underlying_type, TYPE_REFERENCE, indent);
            break;

        case apigen_typeid_function:
            func = type->extra;

            render_type_prefix(stream, func->return_type, TYPE_REFERENCE, indent);
            render_type_suffix(stream, func->return_type, TYPE_REFERENCE, indent);

            apigen_io_printf(stream, " (");
            
            break;

        case apigen_typeid_enum:
            enumeration = type->extra;

            apigen_io_printf(stream, "enum {\n"); 
            for(size_t i = 0; i < enumeration->item_count; i++) {
                struct apigen_EnumItem const item = enumeration->items[i];
                if(item.documentation != NULL) {
                    render_docstring(stream, indent + 1, item.documentation);
                }
                flush_indent(stream, indent + 1);
                render_identifier(stream, ID_UPPERCASE, type->name, false);
                apigen_io_write(stream, "_", 1);
                render_identifier(stream, ID_UPPERCASE, item.name, false);
                apigen_io_printf(stream, " = ");
                if(apigen_type_is_unsigned_integer(enumeration->underlying_type->id)) {
                    apigen_io_printf(stream, "%"PRIu64, item.uvalue);
                }
                else {
                    apigen_io_printf(stream, "%"PRIi64, item.ivalue);
                }
                apigen_io_printf(stream, ",\n", item.name);
                
            }
            flush_indent(stream, indent);
            apigen_io_printf(stream, "}", type->name); 
            break;

        case apigen_typeid_struct:
        case apigen_typeid_union:
            uos = type->extra;

            if(type->id == apigen_typeid_struct) {
                apigen_io_printf(stream, "struct ");
            }
            else {
                apigen_io_printf(stream, "union ");
            }
            render_identifier(stream, ID_KEEP, type->name, true);
            apigen_io_printf(stream, "{\n"); 

            for(size_t i = 0; i < uos->field_count; i++) {
                struct apigen_NamedValue const field = uos->fields[i];

                if(field.documentation != NULL) {
                    render_docstring(stream, indent + 1, field.documentation);
                }
                flush_indent(stream, indent + 1);
                render_declaration(stream, DECL_REGULAR, field.name, ID_LOWERCASE, field.type, TYPE_REFERENCE, indent + 1);
                apigen_io_printf(stream, ";\n");
            }
            flush_indent(stream, indent);
            apigen_io_printf(stream, "}", type->name); 
            break;
        
        case apigen_typeid_alias:
            render_type_prefix(stream, type->extra, TYPE_REFERENCE, indent);
            break;  
        
        case APIGEN_TYPEID_LIMIT: APIGEN_UNREACHABLE();
    }
}

static void render_type_suffix(struct apigen_Stream const stream, struct apigen_Type const * const type, enum RenderMode render_mode, size_t indent)
{
    if((render_mode == TYPE_REFERENCE) && type->name) {
        // no suffix needed
        return;
    }

    switch(type->id)
    {
        struct apigen_Pointer const * pointer;
        struct apigen_Array const * array;
        struct apigen_FunctionType const * func;

        case apigen_typeid_void:        break;
        case apigen_typeid_anyopaque:   break;
        case apigen_typeid_opaque:      break;
        case apigen_typeid_bool:        break;
        case apigen_typeid_uchar:       break;
        case apigen_typeid_ichar:       break;
        case apigen_typeid_char:        break;

        case apigen_typeid_u8:          break;
        case apigen_typeid_u16:         break;
        case apigen_typeid_u32:         break;
        case apigen_typeid_u64:         break;
        case apigen_typeid_usize:       break;
        case apigen_typeid_c_ushort:    break;
        case apigen_typeid_c_uint:      break;
        case apigen_typeid_c_ulong:     break;
        case apigen_typeid_c_ulonglong: break;

        case apigen_typeid_i8:          break;
        case apigen_typeid_i16:         break;
        case apigen_typeid_i32:         break;
        case apigen_typeid_i64:         break;
        case apigen_typeid_isize:       break;
        case apigen_typeid_c_short:     break;
        case apigen_typeid_c_int:       break;
        case apigen_typeid_c_long:      break;
        case apigen_typeid_c_longlong:  break;

        case apigen_typeid_f32:         break;
        case apigen_typeid_f64:         break;

        case apigen_typeid_ptr_to_one:
        case apigen_typeid_ptr_to_many:
        case apigen_typeid_ptr_to_sentinelled_many:
        case apigen_typeid_nullable_ptr_to_one:
        case apigen_typeid_nullable_ptr_to_many:
        case apigen_typeid_nullable_ptr_to_sentinelled_many:
        case apigen_typeid_const_ptr_to_one:
        case apigen_typeid_const_ptr_to_many:
        case apigen_typeid_const_ptr_to_sentinelled_many:
        case apigen_typeid_nullable_const_ptr_to_one:
        case apigen_typeid_nullable_const_ptr_to_many:
        case apigen_typeid_nullable_const_ptr_to_sentinelled_many:
            pointer = type->extra;
            render_type_suffix(stream, pointer->underlying_type, TYPE_REFERENCE, indent);
            break;

        case apigen_typeid_array:
            array = type->extra;
            render_type_suffix(stream, array->underlying_type, TYPE_REFERENCE, indent);
            apigen_io_printf(stream, "[%"PRIu64"]", array->size);
            break;

        case apigen_typeid_function:
            func = type->extra;

            apigen_io_printf(stream, ") "); 
            render_parameter_list(stream, *func, indent);
            
            break;

        case apigen_typeid_enum: 
            break;

        case apigen_typeid_struct:
        case apigen_typeid_union:
            break;
        
        case apigen_typeid_alias:
            render_type_suffix(stream, type->extra, TYPE_REFERENCE, indent);
            break;  
        
        case APIGEN_TYPEID_LIMIT: APIGEN_UNREACHABLE();
    }
}

static void render_declaration(struct apigen_Stream const stream, enum DeclarationKind kind, char const * identifier, enum IdentifierTransform transform, struct apigen_Type const * const type, enum RenderMode render_mode, size_t indent)
{
    APIGEN_NOT_NULL(type);

    render_type_prefix(stream, type, render_mode, indent);
    switch(kind) {
        case DECL_REGULAR: apigen_io_write(stream, " ", 1); break;
        case DECL_CONST:   apigen_io_write(stream, " const ", 7); break;
    }
    render_identifier(stream, transform, identifier, true);  
    
    render_type_suffix(stream, type, render_mode, indent);
}

struct TypeDeclSpec
{
    struct apigen_Type const * type;
    bool requires_forward_decl;
    struct TypeOrderDependency * dependencies;
};  

enum DependencyType {
    DEP_HARD = 0,
    DEP_WEAK = 1,
};

struct TypeOrderDependency
{
    enum DependencyType weakness; // if weak, can be forward-declared, otherwise requries hard decl
    struct apigen_Type const * type; 
    struct TypeOrderDependency * next;
};

static void add_type_dependency(struct apigen_MemoryArena * const arena, struct TypeDeclSpec * const container, struct apigen_Type const * const type, enum DependencyType dep_type)
{
    { // deduplicate or reduce weakness if possible:

        struct TypeOrderDependency * iter = container->dependencies;
        while(iter != NULL) {
            if(iter->type == type) {
                if(iter->weakness > dep_type) {
                    // reference isn't weak, ensure we actually have a non-weak dependency added:
                    iter->weakness = dep_type;
                }
                return;
            }
            iter = iter->next;
        }
    }

    struct TypeOrderDependency * const dep = apigen_memory_arena_alloc(arena, sizeof(struct TypeOrderDependency));
    *dep = (struct TypeOrderDependency) {
        .weakness = dep_type,
        .type = type,
        .next = container->dependencies,
    };
    container->dependencies = dep;
}

static void fetch_dependencies(struct apigen_MemoryArena * const arena, struct TypeDeclSpec * const container, struct apigen_Type const * const type, bool top_level, enum DependencyType dep_weakness)
{
    APIGEN_NOT_NULL(type);
 
    if(!top_level) {
        if(type == container->type) {
            // circular dependency, we can safely ignore it
            return;
        }
        // we can directly depend on named types
        if(type->name != NULL) {
            enum DependencyType dep = dep_weakness;

            // fprintf(stderr, "%s:%d\n", type->name, apigen_type_is_primitive_type(unalias(type)->id));

            if(apigen_type_is_primitive_type(unalias(type)->id) || (type->id == apigen_typeid_alias)) {
                // we cannot forward-declare primitive types, thus we cannot
                // forward declare them, also 
                // we have to depend hard on type aliasing
                dep = DEP_HARD;
            }

            add_type_dependency(arena, container, type, dep);
            return;
        }
    }

    switch(type->id)
    {
        struct apigen_Pointer const * pointer;
        struct apigen_Array const * array;
        struct apigen_FunctionType const * func;

        case apigen_typeid_void:        return;
        case apigen_typeid_anyopaque:   return;
        case apigen_typeid_bool:        return;
        case apigen_typeid_uchar:       return;
        case apigen_typeid_ichar:       return;
        case apigen_typeid_char:        return;

        case apigen_typeid_u8:          return;
        case apigen_typeid_u16:         return;
        case apigen_typeid_u32:         return;
        case apigen_typeid_u64:         return;
        case apigen_typeid_usize:       return;
        case apigen_typeid_c_ushort:    return;
        case apigen_typeid_c_uint:      return;
        case apigen_typeid_c_ulong:     return;
        case apigen_typeid_c_ulonglong: return;

        case apigen_typeid_i8:          return;
        case apigen_typeid_i16:         return;
        case apigen_typeid_i32:         return;
        case apigen_typeid_i64:         return;
        case apigen_typeid_isize:       return;
        case apigen_typeid_c_short:     return;
        case apigen_typeid_c_int:       return;
        case apigen_typeid_c_long:      return;
        case apigen_typeid_c_longlong:  return;

        case apigen_typeid_f32:         return;
        case apigen_typeid_f64:         return;

        case apigen_typeid_opaque:
            if(!top_level) {
                apigen_panic("Cannot implicitly depend on a non-named opaque type");
            }
            break;

        case apigen_typeid_ptr_to_one:
        case apigen_typeid_ptr_to_many:
        case apigen_typeid_ptr_to_sentinelled_many:
        case apigen_typeid_nullable_ptr_to_one:
        case apigen_typeid_nullable_ptr_to_many:
        case apigen_typeid_nullable_ptr_to_sentinelled_many:
        case apigen_typeid_const_ptr_to_one:
        case apigen_typeid_const_ptr_to_many:
        case apigen_typeid_const_ptr_to_sentinelled_many:
        case apigen_typeid_nullable_const_ptr_to_one:
        case apigen_typeid_nullable_const_ptr_to_many:
        case apigen_typeid_nullable_const_ptr_to_sentinelled_many:
            pointer = type->extra;
            fetch_dependencies(arena, container, pointer->underlying_type, false, DEP_WEAK);
            break;

        case apigen_typeid_array:
            array = type->extra;
            fetch_dependencies(arena, container, array->underlying_type, false, DEP_HARD);
            break;

        case apigen_typeid_function:
            func = type->extra;

            fetch_dependencies(arena, container, func->return_type, false, DEP_HARD);
            for(size_t i = 0; i < func->parameter_count; i++)
            {
                struct apigen_NamedValue const param = func->parameters[i];
                fetch_dependencies(arena, container, param.type, false, DEP_HARD);
            }

            break;

        case apigen_typeid_enum: 
            if(!top_level) {
                apigen_panic("Cannot implicitly depend on a non-named enum type");
            }
            break;

        case apigen_typeid_struct:
        case apigen_typeid_union:
            if(!top_level) {
                if(type->id == apigen_typeid_struct) {
                    apigen_panic("Cannot implicitly depend on a non-named struct type");
                } else {
                    apigen_panic("Cannot implicitly depend on a non-named union type");
                }
            }

            struct apigen_UnionOrStruct const * const uos = type->extra;
            for(size_t i = 0; i < uos->field_count; i++) {
                struct apigen_NamedValue const field = uos->fields[i];
                fetch_dependencies(arena, container, field.type, false, DEP_HARD);
            }
            break;

        case apigen_typeid_alias:
            APIGEN_ASSERT(top_level || (type->name != NULL));
            fetch_dependencies(arena, container, type->extra, false, dep_weakness);
            break;
        
        case APIGEN_TYPEID_LIMIT: APIGEN_UNREACHABLE();
    }
}

/// Rotates the given (inclusive) range downwards, and moves the element at `start_incl` to `end_incl`.
static void rotate_decls_down(struct TypeDeclSpec * array, size_t start_incl, size_t end_incl)
{
    APIGEN_NOT_NULL(array);

    struct TypeDeclSpec end = array[start_incl];
    for(size_t i = start_incl; i < end_incl; i++)
    {
        array[i] = array[i + 1];
    }
    array[end_incl] = end;
}

/// Sorts the types from `document` in a way that all hard dependencies are resolved by declaration order,
/// and determines if it's necessary to forward-declare the type as they are used in pointers or similar structures.
static struct TypeDeclSpec * create_type_order_map(struct apigen_MemoryArena * const arena, struct apigen_Document const * const document)
{
    APIGEN_NOT_NULL(arena);
    APIGEN_NOT_NULL(document);

    size_t const array_len = document->type_count;
    struct TypeDeclSpec * const array = apigen_memory_arena_alloc(arena, document->type_count * sizeof(struct TypeDeclSpec));

    // Phase 1: collect all dependencies:
    for(size_t i = 0; i < array_len; i++)
    {
        array[i] = (struct TypeDeclSpec) {
            .type = document->types[i],
            .requires_forward_decl = false,
            .dependencies = NULL,
        };
        fetch_dependencies(arena, &array[i], array[i].type, true, DEP_HARD);
    }

    // Phase 2: resolve dependencies by hard deps
    {
        size_t index = 0;
        while(index < array_len)
        {
            struct TypeDeclSpec const item = array[index];
            size_t dep_first = (array_len - 1);
            size_t dep_last  = 0;
            {
                struct TypeOrderDependency const * iter = item.dependencies;
                while(iter != NULL)
                {
                    if(iter->weakness == DEP_HARD) {
                        bool found = false;
                        for(size_t j = 0; !found && (j < array_len); j++) {
                            if(array[j].type == iter->type) {
                                if(j < dep_first) dep_first = j;
                                if(j > dep_last)  dep_last = j;
                                found = true;
                            }
                        }
                        APIGEN_ASSERT(found); // all dependencies MUST be resolvable, otherwise it's a bug in the analyzer phase
                    }
                    iter = iter->next;
                }
            }
            APIGEN_ASSERT((dep_first >= 0) && (dep_first < array_len));
            APIGEN_ASSERT((dep_last >= 0) && (dep_last < array_len));
            bool const deps_ok = (item.dependencies == NULL) || (dep_last < index);
            
            // fprintf(stderr, "[%3zu] range: [%3zu,%3zu] deps for %s are %s\n", index, dep_first, dep_last, item.type->name, deps_ok ? "ok" : "bad");

            if(deps_ok == false)
            {
                // rotate ourselves beyond our  and try again later,
                // next round uses the next type
                rotate_decls_down(array, index, dep_last);
            }
            else {
                // we're done, continue to the next array element
                index += 1;
            }
        }
    }

    // Phase 3: mark everything left referenced as weak deps as forward required
    for(size_t index = 0; index < array_len; index++)
    {
        struct TypeDeclSpec const item = array[index];

        struct TypeOrderDependency const * iter = item.dependencies;
        while(iter != NULL)
        {
            if(iter->weakness == DEP_WEAK) {
                for(size_t j = index + 1; (j < array_len); j++) {
                    if(array[j].type == iter->type) {
                        array[j].requires_forward_decl = true;
                    }
                }
            }
            iter = iter->next;
        }
    }

    return array;
}

bool apigen_render_c(struct apigen_Stream const stream, struct apigen_MemoryArena * const arena, struct apigen_Diagnostics * const diagnostics, struct apigen_Document const * const document)
{
    APIGEN_NOT_NULL(arena);
    APIGEN_NOT_NULL(diagnostics);
    APIGEN_NOT_NULL(document);

    apigen_io_printf(stream, "%s",
        "#pragma once\n"
        "\n"
        "// THIS IS AUTOGENERATED CODE!\n"
        "\n"
        "#include <stdint.h>\n"
        "#include <stddef.h>\n"
        "#include <stdbool.h>\n"
        "\n"
        "#ifdef __cplusplus\n"
        "extern \"C\" {\n"
        "#endif\n"
        "\n"
    );

    struct TypeDeclSpec const * const ordered_types = create_type_order_map(arena, document);

    // Phase 1: Render necessary forward declarations

    
    for(size_t i = 0; i < document->type_count; i++)
    {
        struct TypeDeclSpec decl = ordered_types[i];
        struct apigen_Type const * const type = decl.type;
        if(decl.requires_forward_decl) {

            switch(unalias(type)->id) {
                case apigen_typeid_enum: apigen_io_write(stream, "enum ", 5); break;
                case apigen_typeid_struct: apigen_io_write(stream, "struct ", 7); break;
                case apigen_typeid_union: apigen_io_write(stream, "union ", 6); break;
                default: APIGEN_UNREACHABLE();
            }
            render_identifier(stream, ID_KEEP, type->name, true);
            apigen_io_printf(stream, ";\n\n");
        }
    }

    // Phase 2: Render concrete type declarations
    for(size_t i = 0; i < document->type_count; i++)
    {
        struct TypeDeclSpec decl = ordered_types[i];
        struct apigen_Type const * const type = decl.type;

        apigen_io_printf(stream, "typedef ");

        render_declaration(stream, DECL_REGULAR, type->name, ID_KEEP, type, TYPE_INSTANCE, 0);

        apigen_io_printf(stream, ";\n\n");
    }

    apigen_io_printf(stream, "\n");

    for(size_t i = 0; i < document->variable_count; i++)
    {
      struct apigen_Global const global = document->variables[i];

      if(global.documentation != NULL) {
        render_docstring(stream, 0, global.documentation);
      }

      apigen_io_write(stream, "extern ", 7);

      render_declaration(stream, global.is_const ? DECL_CONST : DECL_REGULAR, global.name, ID_KEEP, global.type, TYPE_REFERENCE, 0);

      apigen_io_printf(stream, ";\n\n");
    }

    apigen_io_printf(stream, "\n");

    for(size_t i = 0; i < document->constant_count; i++)
    {
      struct apigen_Constant const constant = document->constants[i];

      if(constant.documentation != NULL) {
        render_docstring(stream, 0, constant.documentation);
      }

      apigen_io_printf(stream, "#define ");
      render_identifier(stream, ID_UPPERCASE, constant.name, true);
      apigen_io_printf(stream, " ");
      render_value(stream, constant.value);
      apigen_io_printf(stream, " // ");
      render_type_prefix(stream, constant.type, TYPE_REFERENCE, 0); // TODO: These might do line breaks when constants have weird types!
      render_type_suffix(stream, constant.type, TYPE_REFERENCE, 0); // TODO: These might do line breaks when constants have weird types!
      apigen_io_printf(stream, "\n\n");
    }

    apigen_io_printf(stream, "\n");

    for(size_t i = 0; i < document->function_count; i++)
    {
        struct apigen_Function func = document->functions[i];

        if(func.documentation != NULL) {
            render_docstring(stream, 0, func.documentation);
        }

        render_declaration(stream, DECL_REGULAR, func.name, ID_KEEP, func.type, TYPE_INSTANCE, 0);

        apigen_io_printf(stream, ";\n\n");
    }

    apigen_io_printf(stream, "%s",
        "\n"
        "#ifdef __cplusplus\n"
        "} // ends extern \"C\"\n"
        "#endif\n"
        "\n"
    );

    return true;
}


bool apigen_render_cpp(struct apigen_Stream stream, struct apigen_MemoryArena * arena, struct apigen_Diagnostics * diagnostics, struct apigen_Document const * document)
{
  APIGEN_NOT_NULL(arena);
  APIGEN_NOT_NULL(diagnostics);
  APIGEN_NOT_NULL(document);

  (void) stream;

  return false;
}
