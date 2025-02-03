#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>

/*
 * A simple memory management interface.
 * You can replace these with GC or reference-counted allocations.
 */

void* memory_allocate(size_t size);
void  memory_free(void* ptr);
void* memory_reallocate(void* ptr, size_t new_size);

#endif /* MEMORY_H */
