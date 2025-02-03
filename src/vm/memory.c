#include "memory.h"
#include <stdlib.h>
#include <stdio.h>

void* memory_allocate(size_t size) {
    void* ptr = malloc(size);
    if (!ptr && size != 0) {
        fprintf(stderr, "memory_allocate: out of memory (requested %zu bytes)\n", size);
        /* handle error or exit. */
    }
    return ptr;
}

void memory_free(void* ptr) {
    free(ptr);
}

void* memory_reallocate(void* ptr, size_t new_size) {
    void* new_ptr = realloc(ptr, new_size);
    if (!new_ptr && new_size != 0) {
        fprintf(stderr, "memory_reallocate: out of memory (requested %zu bytes)\n", new_size);
        /* handle error or exit. */
    }
    return new_ptr;
}
