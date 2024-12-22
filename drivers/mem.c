#include <stdint.h>

#include "mem.h"

// Assumes that sizeof(type) is a power of 2
#define ROUND(x, type) (((x) + sizeof(type) - 1) & ~(sizeof(type) - 1))

static size_t heapdata[HEAP_SIZE / sizeof(size_t)];
#define HEAPEND ((void*)(((uint8_t*)heapdata) + sizeof(heapdata)))

struct heap_header_t
{
    size_t size;
};

void mem_init()
{
    heapdata[0] = HEAP_SIZE;
}

void *malloc(size_t size)
{
    size = ROUND(size + sizeof(struct heap_header_t), size_t);

    void *ptr = heapdata;
    while(ptr < HEAPEND)
    {
        struct heap_header_t *header = ptr;
        size_t header_size = header->size;
        if(header_size >= size)
        {
            header->size = size;
            struct heap_header_t *next = (struct heap_header_t*)((uint8_t*)ptr + size);
            next->size = header_size - size;
            return ((uint8_t*)ptr + sizeof(*header));
        }

        ptr = (uint8_t*)ptr + header->size;
    }
    return NULL;
}

