#include "memory.h"
#include <stdlib.h>

void* memory_allocate(size_t size) {
    return malloc(size);
}

void memory_free(void* ptr) {
    free(ptr);
}
