#include "apigen.h"


bool apigen_render_c(struct apigen_Stream stream, struct apigen_MemoryArena * arena, struct apigen_Diagnostics * diagnostics, struct apigen_Document const * document)
{
  APIGEN_NOT_NULL(arena);
  APIGEN_NOT_NULL(diagnostics);
  APIGEN_NOT_NULL(document);

  (void) stream;

  return false;
}

bool apigen_render_cpp(struct apigen_Stream stream, struct apigen_MemoryArena * arena, struct apigen_Diagnostics * diagnostics, struct apigen_Document const * document)
{
  APIGEN_NOT_NULL(arena);
  APIGEN_NOT_NULL(diagnostics);
  APIGEN_NOT_NULL(document);

  (void) stream;

  return false;
}
