#include "apigen.h"

#include <string.h>
#include <inttypes.h>

enum RenderMode {
  TYPE_REFERENCE,
  TYPE_INSTANCE,
};

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

static void render_identifier(struct apigen_Stream const stream, char const * identifier, bool exact_match)
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

  for(size_t i = 0; reserved_identifiers[i]; i++) {
    if(apigen_streq(identifier, reserved_identifiers[i])) {
      reserved = true;
      break;
    }
  }
  if(reserved) {
    if(exact_match) {
      apigen_panic("used unrecoverable reserved identifier!");
    }
    apigen_io_printf(stream, "%s_", identifier);
  }
  else {
    apigen_io_printf(stream, "%s", identifier);
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

static void render_type(struct apigen_Stream const stream, struct apigen_Type const * const type, enum RenderMode render_mode, size_t indent);

static void render_func_signature(struct apigen_Stream const stream, struct apigen_FunctionType func, size_t indent)
{
  apigen_io_printf(stream, "(\n"); 
  for(size_t i = 0; i < func.parameter_count; i++)
  {
    struct apigen_NamedValue const param = func.parameters[i];

    if(param.documentation != NULL) {
      render_docstring(stream, indent + 1, param.documentation);
    }
    flush_indent(stream, indent + 1);
    render_type(stream, param.type, TYPE_REFERENCE, indent + 1);
    apigen_io_printf(stream, " ");
    render_identifier(stream, param.name, false);

    if(i + 1 == func.parameter_count) {
      apigen_io_printf(stream, "\n");
    }
    else {
      apigen_io_printf(stream, ",\n");
    }
  }
  flush_indent(stream, indent);
  apigen_io_printf(stream, ") "); 
  
  render_type(stream, func.return_type, TYPE_REFERENCE, indent);
}

static void render_type(struct apigen_Stream const stream, struct apigen_Type const * const type, enum RenderMode render_mode, size_t indent)
{
  APIGEN_NOT_NULL(type);

  if((render_mode == TYPE_REFERENCE) && type->name) {

    switch(type->id) {
      case apigen_typeid_struct: apigen_io_write(stream, "struct ", 7); break;
      case apigen_typeid_union:  apigen_io_write(stream, "union ", 6); break;
      default: break;
    }

    apigen_io_printf(stream, "%s", type->name); 
    return;
  }

  switch(type->id)
  {
    struct apigen_Pointer const * pointer;
    struct apigen_Array const * array;
    struct apigen_FunctionType const * func;
    struct apigen_Enum const * enumeration;

    case apigen_typeid_void:        apigen_io_printf(stream, "void"); break;
    case apigen_typeid_anyopaque:   apigen_io_printf(stream, "void"); break;
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
    case apigen_typeid_c_longlong:  apigen_io_printf(stream, "longlong"); break;

    case apigen_typeid_f32:         apigen_io_printf(stream, "float"); break;
    case apigen_typeid_f64:         apigen_io_printf(stream, "double"); break;



    case apigen_typeid_ptr_to_one:
    case apigen_typeid_ptr_to_many:
    case apigen_typeid_ptr_to_sentinelled_many:
    case apigen_typeid_nullable_ptr_to_one:
    case apigen_typeid_nullable_ptr_to_many:
    case apigen_typeid_nullable_ptr_to_sentinelled_many:
      pointer = type->extra;
      render_type(stream, pointer->underlying_type, TYPE_REFERENCE, indent);
      apigen_io_printf(stream, " *");
      break;

    case apigen_typeid_const_ptr_to_one:
    case apigen_typeid_const_ptr_to_many:
    case apigen_typeid_const_ptr_to_sentinelled_many:
    case apigen_typeid_nullable_const_ptr_to_one:
    case apigen_typeid_nullable_const_ptr_to_many:
    case apigen_typeid_nullable_const_ptr_to_sentinelled_many:
      pointer = type->extra;
      render_type(stream, pointer->underlying_type, TYPE_REFERENCE, indent);
      apigen_io_printf(stream, " const *");
      break;


    case apigen_typeid_array:
      array = type->extra;
      apigen_io_printf(stream, "[%"PRIu64"]", array->size);
      render_type(stream, array->underlying_type, TYPE_REFERENCE, indent);
      break;

    case apigen_typeid_function:
      func = type->extra;

      apigen_io_printf(stream, "fn"); 
      render_func_signature(stream, *func, indent);
      
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
        render_identifier(stream, item.name, true);
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
      if(render_mode == TYPE_REFERENCE) {
        apigen_io_printf(stream, "%s", type->name); 
        break;
      }

      struct apigen_UnionOrStruct const * const uos = type->extra;

      if(type->id == apigen_typeid_struct) {
        apigen_io_printf(stream, "struct ");
      }
      else {
        apigen_io_printf(stream, "union ");
      }
      render_identifier(stream, type->name, true);
      apigen_io_printf(stream, "{\n"); 

      for(size_t i = 0; i < uos->field_count; i++) {
        struct apigen_NamedValue const field = uos->fields[i];

        if(field.documentation != NULL) {
          render_docstring(stream, indent + 1, field.documentation);
        }
        flush_indent(stream, indent + 1);
        render_type(stream, field.type, TYPE_REFERENCE, indent + 1);
        apigen_io_printf(stream, " ");
        render_identifier(stream, field.name, false);
        apigen_io_printf(stream, ";\n");
      }
      flush_indent(stream, indent);
      apigen_io_printf(stream, "}", type->name); 
      break;
     
    case apigen_typeid_opaque:
      apigen_io_printf(stream, "opaque{}");
      break;    
      
    case apigen_typeid_alias:
      render_type(stream, type->extra, TYPE_REFERENCE, indent);
      break;  
    
    case APIGEN_TYPEID_LIMIT: APIGEN_UNREACHABLE();
  }
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

  for(size_t i = 0; i < document->type_count; i++)
  {
    struct apigen_Type const * const type = document->types[i];

    apigen_io_printf(stream, "typedef ");
    render_type(stream, type, TYPE_INSTANCE, 0);
    apigen_io_printf(stream, " ");
    render_identifier(stream, type->name, true);
    apigen_io_printf(stream, ";\n\n");
  }
  
  apigen_io_printf(stream, "\n");

  // for(size_t i = 0; i < document->variable_count; i++)
  // {
  //   struct apigen_Global const global = document->variables[i];

  //   if(global.documentation != NULL) {
  //     render_docstring(stream, 0, global.documentation);
  //   }

  //   apigen_io_printf(stream, "pub extern %s ", global.is_const ? "const" : "var");
  //   render_identifier(stream, global.name, true);
  //   apigen_io_printf(stream, ": ");
  //   render_type(stream, global.type, TYPE_REFERENCE, 0);
  //   apigen_io_printf(stream, ";\n\n");
  // }
  
  // apigen_io_printf(stream, "\n");

  // for(size_t i = 0; i < document->constant_count; i++)
  // {
  //   struct apigen_Constant const constant = document->constants[i];


  //   if(constant.documentation != NULL) {
  //     render_docstring(stream, 0, constant.documentation);
  //   }

  //   apigen_io_printf(stream, "pub const ");
  //   render_identifier(stream, constant.name, true);
  //   apigen_io_printf(stream, ": ");
  //   render_type(stream, constant.type, TYPE_REFERENCE, 0);
  //   apigen_io_printf(stream, " = ");
  //   render_value(stream, constant.value);
  //   apigen_io_printf(stream, ";\n\n");
  // }

  // apigen_io_printf(stream, "\n");

  // for(size_t i = 0; i < document->function_count; i++)
  // {
  //   struct apigen_Function func = document->functions[i];

  //   if(func.documentation != NULL) {
  //     render_docstring(stream, 0, func.documentation);
  //   }
    
  //   apigen_io_printf(stream, "pub extern fn ");
  //   render_identifier(stream, func.name, true);

  //   render_func_signature(stream, *((struct apigen_FunctionType const *)(func.type->extra)), 0);

  //   apigen_io_printf(stream, ";\n\n");
  // }

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
