#include "apigen.h"

#include <stdio.h>
#include <stdlib.h>

extern void * (* apigen_memory_alloc_backend)(size_t);
extern void (*apigen_memory_free_backend)(void*);

static size_t allocation_count = 0;
static void * test_malloc(size_t size)
{
    void * ptr = malloc(size);
    if(ptr != NULL) {
        allocation_count += 1;
    }
    return ptr;
}

static void test_free(void *ptr)
{
    if(ptr) {
        APIGEN_ASSERT(allocation_count > 0);
        allocation_count -= 1;
    }
    free(ptr);
}

int main(int argc, char ** argv)
{
    (void)argc;
    (void)argv;

    apigen_memory_alloc_backend = test_malloc;
    apigen_memory_free_backend = test_free;

    {
        struct apigen_MemoryArena arena;
        apigen_memory_arena_init(&arena);
        apigen_memory_arena_deinit(&arena);
    }

    APIGEN_ASSERT(allocation_count == 0);

    // test many small allocations
    {
        struct apigen_MemoryArena arena;
        apigen_memory_arena_init(&arena);

        for(size_t i = 0; i < 48; i++) {
            (void)apigen_memory_arena_alloc(&arena, 64);
        }

        apigen_memory_arena_deinit(&arena);
    }

    APIGEN_ASSERT(allocation_count == 0);

    // test some larger allocations
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

    APIGEN_ASSERT(allocation_count == 0);

    return 0;
}
