#include "apigen.h"
#include "unittest.h"


UNITTEST("Arena: basic init/deinit")
{
    struct apigen_MemoryArena arena;
    apigen_memory_arena_init(&arena);
    apigen_memory_arena_deinit(&arena);
}

UNITTEST("Arena: Small allocs")
{
    struct apigen_MemoryArena arena;
    apigen_memory_arena_init(&arena);

    for(size_t i = 0; i < 48; i++) {
        (void)apigen_memory_arena_alloc(&arena, 64);
    }

    apigen_memory_arena_deinit(&arena);
}

UNITTEST("Arena: large allocations")
{
    struct apigen_MemoryArena arena;
    apigen_memory_arena_init(&arena);

    size_t alloc_size = arena.chunk_size / 4;

    for(size_t i = 0; i < 8; i++) {
        (void)apigen_memory_arena_alloc(&arena, alloc_size);
        alloc_size *= 2; // increment each step
    }

    apigen_memory_arena_deinit(&arena);
}

