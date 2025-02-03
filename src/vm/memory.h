#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>

/**
 * A simple memory management interface.
 * 
 * This can wrap malloc/free, or be replaced with a custom allocator,
 * garbage collector, reference-counting system, etc.
 */

void* memory_allocate(size_t size);
void memory_free(void* ptr);

/**
 * Optional function: reallocate an existing pointer with a new size.
 * This is a convenience wrapper over your chosen memory system.
 */
void* memory_reallocate(void* ptr, size_t new_size);

#endif // MEMORY_H
