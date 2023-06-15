#include "apigen.h"

#include <stdlib.h>
#include <string.h>
#include <stdalign.h>

static size_t maxSize(size_t a, size_t b) 
{
    return (a>b) ? a : b;
}

static bool isAligned(uintptr_t addr, size_t const alignment)
{
    APIGEN_ASSERT((alignment & (alignment - 1)) == 0);
    
    return ((addr & (alignment-1)) == 0);
}

static bool isAddrAligned(void const * const ptr, size_t const alignment)
{
    uintptr_t const addr = (uintptr_t)(ptr);
    return isAligned(addr, alignment);
}


static size_t alignSizeForward(size_t const original_size)
{
    size_t const aligned_size = (original_size + alignof(max_align_t) - 1) & ~(alignof(max_align_t) - 1U);
    APIGEN_ASSERT(aligned_size >= original_size);
    isAligned(aligned_size, alignof(max_align_t));
    return aligned_size;
}

struct apigen_memory_arena_chunk
{
    char * memory;
    size_t size;
    struct apigen_memory_arena_chunk * next;
};

void * (* apigen_memory_alloc_backend)(size_t) = malloc;
void (*apigen_memory_free_backend)(void*) = free;

void * apigen_alloc(size_t size)
{
    void * ptr = apigen_memory_alloc_backend(size);
    if(ptr == NULL)
        apigen_panic("out of memory");
    APIGEN_ASSERT(isAddrAligned(ptr, alignof(max_align_t)));
    memset(ptr, 0xAA, size);
    return ptr;
}

void apigen_free(void * ptr)
{
    apigen_memory_free_backend(ptr);
}

// arena APIs:

void apigen_memory_arena_init(struct apigen_memory_arena * arena)
{
    *arena = (struct apigen_memory_arena) {
        .first_chunk = NULL,
        .last_chunk = NULL,
        .chunk_size = 1024,
    };
}

void apigen_memory_arena_deinit(struct apigen_memory_arena * arena)
{
    struct apigen_memory_arena_chunk * chunk = arena->first_chunk;
    while(chunk) {
        struct apigen_memory_arena_chunk * const to_be_deleted = chunk;
        chunk = chunk->next;
        apigen_free (to_be_deleted);
    }
    memset(arena, 0xAA, sizeof *arena);
}

void * apigen_memory_arena_alloc(struct apigen_memory_arena * arena, size_t size)
{
    size_t const aligned_size = alignSizeForward(size);

    struct apigen_memory_arena_chunk * chunk = arena->last_chunk;

    if(chunk == NULL || chunk->size < aligned_size) {
        size_t const chunk_aligned_size = alignSizeForward(sizeof(struct apigen_memory_arena_chunk));
        
        size_t const new_chunk_size = maxSize(arena->chunk_size, aligned_size);

        chunk = apigen_alloc(chunk_aligned_size + new_chunk_size);
        *chunk = (struct apigen_memory_arena_chunk) {
            .memory = ((char*)chunk) + chunk_aligned_size,
            .size = new_chunk_size,
            .next = NULL,
        };

        if(arena->last_chunk == NULL) {
            APIGEN_ASSERT(arena->first_chunk == NULL);

            arena->last_chunk = chunk;
            arena->first_chunk = chunk;
        }
        else {
            APIGEN_ASSERT(arena->first_chunk != NULL);
            arena->last_chunk->next = chunk;
            arena->last_chunk = chunk;
        }
    }

    void * const alloc_ptr = chunk->memory;

    APIGEN_ASSERT(chunk->size >= aligned_size);

    chunk->memory += aligned_size;
    chunk->size -= aligned_size;

    APIGEN_ASSERT(isAddrAligned(alloc_ptr, alignof(max_align_t)));

    return alloc_ptr;
}

char *apigen_memory_arena_dupestr(struct apigen_memory_arena *arena, char const * str)
{
    size_t len = strlen(str);
    char * res = apigen_memory_arena_alloc(arena, len+1);
    memcpy(res, str,len);
    res[len] = 0;
    return res;
}
