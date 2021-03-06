#ifndef DRIVERS_MEM_H
#define DRIVERS_MEM_H

#include <stddef.h>

#define HEAP_SIZE (16lu << 20)

void *malloc(size_t size);

#endif

