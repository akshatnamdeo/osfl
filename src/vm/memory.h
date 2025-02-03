#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>

/* A simple memory management interface.
   For now, it directly wraps malloc/free. In a more advanced implementation,
   you could add pooling, garbage collection, etc.
*/
void* memory_allocate(size_t size);
void memory_free(void* ptr);

#endif // MEMORY_H
