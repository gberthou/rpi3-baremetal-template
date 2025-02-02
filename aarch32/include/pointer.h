#ifndef AARCH32_POINTER_H
#define AARCH32_POINTER_H

#include <stdint.h>

static inline uint32_t ptr_to_u32(const void* ptr)
{
    return (uint32_t) ptr;
}

static inline uint32_t vptr_to_u32(const volatile void* ptr)
{
    return (uint32_t) ptr;
}
#define VPTR_TO_U32(ptr) ((uint32_t)(ptr))

static inline void *u32_to_ptr(uint32_t x)
{
    return (void*) x;
}

static inline uint32_t ptr_to_integer(const void *ptr)
{
    return (uint32_t) ptr;
}

#endif // AARCH32_POINTER_H

