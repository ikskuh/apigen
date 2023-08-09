#include "apigen.h"

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
  APIGEN_NOT_NULL(docstring);
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

static void render_identifier(struct apigen_Stream const stream, char const * identifier)
{
  static char const * const reserved_identifiers[] = {
    "addrspace",      "align",          "allowzero",      "and",            "anyframe",       "anytype",        "asm",
    "async",          "await",          "break",          "callconv",       "catch",          "comptime",       "const",
    "continue",       "defer",          "else",           "enum",           "errdefer",       "error",          "export",
    "extern",         "fn",             "for",            "if",             "inline",         "noalias",        "nosuspend",
    "noinline",       "opaque",         "or",             "orelse",         "packed",         "pub",            "resume",
    "return",         "linksection",    "struct",         "suspend",        "switch",         "test",           "threadlocal",
    "try",            "union",          "unreachable",    "usingnamespace", "var",            "volatile",       "while",
    NULL,
  };

  for(size_t i = 0; reserved_identifiers[i]; i++) {
    if(apigen_streq(identifier, reserved_identifiers[i])) {
      apigen_io_printf(stream, "@\"%s\"", identifier);
      return;
    }
  }
  apigen_io_print(stream, identifier);
}

static void render_value(struct apigen_Stream const stream, struct apigen_Value value)
{
  switch(value.type) {
    case apigen_value_null: apigen_io_print(stream, "null"); break;
    case apigen_value_sint: apigen_io_printf(stream, "%"PRIi64, value.value_sint); break;
    case apigen_value_uint: apigen_io_printf(stream, "%"PRIu64, value.value_uint); break;
    case apigen_value_str: apigen_io_printf(stream, "\"%s\"", value.value_str); break;
  }
}

static void render_type(struct apigen_Stream const stream, struct apigen_Type const * const type, enum RenderMode render_mode, size_t indent);

static void render_func_signature(struct apigen_Stream const stream, struct apigen_FunctionType func, size_t indent)
{
  apigen_io_print(stream, "(\n");
  for(size_t i = 0; i < func.parameter_count; i++)
  {
    struct apigen_NamedValue const param = func.parameters[i];

    if(param.documentation != NULL) {
      render_docstring(stream, indent + 1, param.documentation);
    }
    flush_indent(stream, indent + 1);
    render_identifier(stream, param.name);
    apigen_io_print(stream, ": ");
    render_type(stream, param.type, TYPE_REFERENCE, indent + 1);
    apigen_io_print(stream, ",\n");
  }
  flush_indent(stream, indent);
  apigen_io_print(stream, ") ");

  render_type(stream, func.return_type, TYPE_REFERENCE, indent);
}

static void render_type(struct apigen_Stream const stream, struct apigen_Type const * const type, enum RenderMode render_mode, size_t indent)
{
  APIGEN_NOT_NULL(type);

  if((render_mode == TYPE_REFERENCE) && type->name) {
    apigen_io_print(stream, type->name);
    return;
  }

  struct apigen_Pointer const * pointer;
  struct apigen_Array const * array;
  struct apigen_FunctionType const * func;
  struct apigen_Enum const * enumeration;
  switch(type->id)
  {

    case apigen_typeid_void:        apigen_io_print(stream, "void"); break;
    case apigen_typeid_anyopaque:   apigen_io_print(stream, "anyopaque"); break;
    case apigen_typeid_bool:        apigen_io_print(stream, "bool"); break;
    case apigen_typeid_uchar:       apigen_io_print(stream, "u8"); break;
    case apigen_typeid_ichar:       apigen_io_print(stream, "i8"); break;
    case apigen_typeid_char:        apigen_io_print(stream, "u8"); break;

    case apigen_typeid_u8:          apigen_io_print(stream, "u8"); break;
    case apigen_typeid_u16:         apigen_io_print(stream, "u16"); break;
    case apigen_typeid_u32:         apigen_io_print(stream, "u32"); break;
    case apigen_typeid_u64:         apigen_io_print(stream, "u64"); break;
    case apigen_typeid_usize:       apigen_io_print(stream, "usize"); break;
    case apigen_typeid_c_ushort:    apigen_io_print(stream, "c_ushort"); break;
    case apigen_typeid_c_uint:      apigen_io_print(stream, "c_uint"); break;
    case apigen_typeid_c_ulong:     apigen_io_print(stream, "c_ulong"); break;
    case apigen_typeid_c_ulonglong: apigen_io_print(stream, "c_ulonglong"); break;

    case apigen_typeid_i8:          apigen_io_print(stream, "i8"); break;
    case apigen_typeid_i16:         apigen_io_print(stream, "i16"); break;
    case apigen_typeid_i32:         apigen_io_print(stream, "i32"); break;
    case apigen_typeid_i64:         apigen_io_print(stream, "i64"); break;
    case apigen_typeid_isize:       apigen_io_print(stream, "isize"); break;
    case apigen_typeid_c_short:     apigen_io_print(stream, "c_short"); break;
    case apigen_typeid_c_int:       apigen_io_print(stream, "c_int"); break;
    case apigen_typeid_c_long:      apigen_io_print(stream, "c_long"); break;
    case apigen_typeid_c_longlong:  apigen_io_print(stream, "c_longlong"); break;

    case apigen_typeid_f32:         apigen_io_print(stream, "f32"); break;
    case apigen_typeid_f64:         apigen_io_print(stream, "f64"); break;



    case apigen_typeid_ptr_to_one:
      pointer = type->extra;
      apigen_io_print(stream, "*");
      render_type(stream, pointer->underlying_type, TYPE_REFERENCE, indent);
      break;

    case apigen_typeid_ptr_to_many:
      pointer = type->extra;
      apigen_io_print(stream, "[*]");
      render_type(stream, pointer->underlying_type, TYPE_REFERENCE, indent);
      break;

    case apigen_typeid_ptr_to_sentinelled_many:
      pointer = type->extra;
      apigen_io_print(stream, "[*:");
      render_value(stream, pointer->sentinel);
      apigen_io_print(stream, "]");
      render_type(stream, pointer->underlying_type, TYPE_REFERENCE, indent);
      break;

    case apigen_typeid_nullable_ptr_to_one:
      pointer = type->extra;
      apigen_io_print(stream, "?*");
      render_type(stream, pointer->underlying_type, TYPE_REFERENCE, indent);
      break;

    case apigen_typeid_nullable_ptr_to_many:
      pointer = type->extra;
      apigen_io_print(stream, "?[*]");
      render_type(stream, pointer->underlying_type, TYPE_REFERENCE, indent);
      break;

    case apigen_typeid_nullable_ptr_to_sentinelled_many:
      pointer = type->extra;
      apigen_io_print(stream, "?[*:");
      render_value(stream, pointer->sentinel);
      apigen_io_print(stream, "]");
      render_type(stream, pointer->underlying_type, TYPE_REFERENCE, indent);
      break;

    case apigen_typeid_const_ptr_to_one:
      pointer = type->extra;
      apigen_io_print(stream, "*const ");
      render_type(stream, pointer->underlying_type, TYPE_REFERENCE, indent);
      break;

    case apigen_typeid_const_ptr_to_many:
      pointer = type->extra;
      apigen_io_print(stream, "[*]const ");
      render_type(stream, pointer->underlying_type, TYPE_REFERENCE, indent);
      break;

    case apigen_typeid_const_ptr_to_sentinelled_many:
      pointer = type->extra;
      apigen_io_print(stream, "[*:");
      render_value(stream, pointer->sentinel);
      apigen_io_print(stream, "]const ");
      render_type(stream, pointer->underlying_type, TYPE_REFERENCE, indent);
      break;

    case apigen_typeid_nullable_const_ptr_to_one:
      pointer = type->extra;
      apigen_io_print(stream, "?*const ");
      render_type(stream, pointer->underlying_type, TYPE_REFERENCE, indent);
      break;

    case apigen_typeid_nullable_const_ptr_to_many:
      pointer = type->extra;
      apigen_io_print(stream, "?[*]const ");
      render_type(stream, pointer->underlying_type, TYPE_REFERENCE, indent);
      break;

    case apigen_typeid_nullable_const_ptr_to_sentinelled_many:
      pointer = type->extra;
      apigen_io_print(stream, "?[*:");
      render_value(stream, pointer->sentinel);
      apigen_io_print(stream, "]const ");
      render_type(stream, pointer->underlying_type, TYPE_REFERENCE, indent);
      break;


    case apigen_typeid_array:
      array = type->extra;
      apigen_io_printf(stream, "[%"PRIu64"]", array->size);
      render_type(stream, array->underlying_type, TYPE_REFERENCE, indent);
      break;

    case apigen_typeid_function:
      func = type->extra;

      apigen_io_print(stream, "fn");
      render_func_signature(stream, *func, indent);

      break;

    case apigen_typeid_enum:
      enumeration = type->extra;

      apigen_io_print(stream, "enum(");
      render_type(stream, enumeration->underlying_type, TYPE_REFERENCE, indent);
      apigen_io_print(stream, ") {\n");
      for(size_t i = 0; i < enumeration->item_count; i++) {
        struct apigen_EnumItem const item = enumeration->items[i];
        if(item.documentation != NULL) {
          render_docstring(stream, indent + 1, item.documentation);
        }
        flush_indent(stream, indent + 1);
        render_identifier(stream, item.name);
        apigen_io_print(stream, " = ");
        if(apigen_type_is_unsigned_integer(enumeration->underlying_type->id)) {
          apigen_io_printf(stream, "%"PRIu64, item.uvalue);
        }
        else {
          apigen_io_printf(stream, "%"PRIi64, item.ivalue);
        }
        apigen_io_print(stream, ",\n");

      }
        flush_indent(stream, indent);
      apigen_io_print(stream, "}");
      break;

    case apigen_typeid_struct:
    case apigen_typeid_union:
      if(render_mode == TYPE_REFERENCE) {
        apigen_io_print(stream, type->name);
        break;
      }

      struct apigen_UnionOrStruct const * const uos = type->extra;

      if(type->id == apigen_typeid_struct) {
        apigen_io_print(stream, "extern struct {\n");
      }
      else {
        apigen_io_print(stream, "extern union {\n");
      }

      for(size_t i = 0; i < uos->field_count; i++) {
        struct apigen_NamedValue const field = uos->fields[i];

        if(field.documentation != NULL) {
          render_docstring(stream, indent + 1, field.documentation);
        }
        flush_indent(stream, indent + 1);
        render_identifier(stream, field.name);
        apigen_io_print(stream, ": ");
        render_type(stream, field.type, TYPE_REFERENCE, indent + 1);
        apigen_io_print(stream, ",\n");
      }
      flush_indent(stream, indent);
      apigen_io_print(stream, "}");
      break;

    case apigen_typeid_opaque:
      apigen_io_print(stream, "opaque{}");
      break;

    case apigen_typeid_alias:
      render_type(stream, type->extra, TYPE_REFERENCE, indent);
      break;

    case APIGEN_TYPEID_LIMIT: APIGEN_UNREACHABLE();
  }
}





bool apigen_render_zig(struct apigen_Stream const stream, struct apigen_MemoryArena * const arena, struct apigen_Diagnostics * const diagnostics, struct apigen_Document const * const document)
{
  APIGEN_NOT_NULL(arena);
  APIGEN_NOT_NULL(diagnostics);
  APIGEN_NOT_NULL(document);

  apigen_io_print(stream, "//! THIS IS AUTOGENERATED CODE!\n\n");

  for(size_t i = 0; i < document->type_count; i++)
  {
    struct apigen_Type const * const type = document->types[i];

    apigen_io_print(stream, "pub const ");
    render_identifier(stream, type->name);
    apigen_io_print(stream, " = ");
    render_type(stream, type, TYPE_INSTANCE, 0);
    apigen_io_print(stream, ";\n\n");
  }

  apigen_io_print(stream, "\n");

  for(size_t i = 0; i < document->variable_count; i++)
  {
    struct apigen_Global const global = document->variables[i];

    if(global.documentation != NULL) {
      render_docstring(stream, 0, global.documentation);
    }

    apigen_io_printf(stream, "pub extern %s ", global.is_const ? "const" : "var");
    render_identifier(stream, global.name);
    apigen_io_print(stream, ": ");
    render_type(stream, global.type, TYPE_REFERENCE, 0);
    apigen_io_print(stream, ";\n\n");
  }

  apigen_io_print(stream, "\n");

  for(size_t i = 0; i < document->constant_count; i++)
  {
    struct apigen_Constant const constant = document->constants[i];


    if(constant.documentation != NULL) {
      render_docstring(stream, 0, constant.documentation);
    }

    apigen_io_print(stream, "pub const ");
    render_identifier(stream, constant.name);
    apigen_io_print(stream, ": ");
    render_type(stream, constant.type, TYPE_REFERENCE, 0);
    apigen_io_print(stream, " = ");
    render_value(stream, constant.value);
    apigen_io_print(stream, ";\n\n");
  }

  apigen_io_print(stream, "\n");

  for(size_t i = 0; i < document->function_count; i++)
  {
    struct apigen_Function func = document->functions[i];

    if(func.documentation != NULL) {
      render_docstring(stream, 0, func.documentation);
    }

    apigen_io_print(stream, "pub extern fn ");
    render_identifier(stream, func.name);

    render_func_signature(stream, *((struct apigen_FunctionType const *)(func.type->extra)), 0);

    apigen_io_print(stream, ";\n\n");
  }

  return true;
}
