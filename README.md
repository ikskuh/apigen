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

## Backends

- [ ] C
- [ ] C++
- [ ] Zig
- [ ] Rust
- [ ] Go

## Missing features

- [ ] Field alignment
- [ ] Pointer alignment
- [ ] Macro/inline function support
- [ ] Implementation stub generator
- [ ] Packed structs
- [ ] Bitfield generation (Zig: `packed struct`, C: int + macro helpers, Rust: ???)
- [ ] Namespaces

## Building

### Dependencies

- [`zig`](https://ziglang.org/download/), at least 0.11.0-dev.3258 or later
- [`flex`](https://github.com/westes/flex), at least version 2.6.4
- [`bison`](https://www.gnu.org/software/bison/), at least version 3.8.2

### Build Commands

### Build

```sh-session
abc@ce7d9be5b94d:/workspace/projects/apigen$ zig build install
abc@ce7d9be5b94d:/workspace/projects/apigen$ 
```

will output `zig-out/bin/apigen`

### Tests

```sh-session
abc@ce7d9be5b94d:/workspace/projects/apigen$ zig build test
abc@ce7d9be5b94d:/workspace/projects/apigen$ 
```

will run the test suite and outputs the failed tests, if any.

