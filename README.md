# APIGEN - API Definition Language

APIGEN provides tooling to create a single-source-of-truth API definition and generate
bindings and implementation stubs from it.

The project goal is that developers don't have to maintain the guts of library wrappers
anymore and provide a precise way to declare APIs:

```zig
type apigen_TypeId = enum {
    apigen_typeid_void,
    apigen_typeid_anyopaque,
    apigen_typeid_opaque,

    // ...
};

type apigen_Type = struct {
  /// What kind of type is this type.
  id: apigen_TypeId,

  /// Points to extra information to interpret this type.
  extra: ?*anyopaque,

  /// Non-NULL if the type has was explicitly declared with a name. Otherwise, this type is anonymous
  name: ?[*:0]const char,
};

type apigen_MemoryArenaChunk = opaque{};

type apigen_MemoryArena = struct {
  first_chunk: ?*apigen_MemoryArenaChunk,
  last_chunk: ?*apigen_MemoryArenaChunk,
  chunk_size: usize,
};

fn apigen_memory_arena_init(arena: *apigen_MemoryArena) void;
fn apigen_memory_arena_deinit(arena: *apigen_MemoryArena) void;
```

## Missing features

- [ ] Field alignment
- [ ] Pointer alignment
- [ ] Macro/inline function support
