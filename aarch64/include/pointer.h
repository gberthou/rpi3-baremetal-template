#ifndef AARCH64_POINTER_H
#define AARCH64_POINTER_H

#include <stdint.h>

static inline uint32_t ptr_to_u32(const void* ptr)
{
    uint64_t x = (uint64_t) ptr;
    return x;
}

static inline uint32_t vptr_to_u32(const volatile void* ptr)
{
    uint64_t x = (uint64_t) ptr;
    return x;
}
#define VPTR_TO_U32(ptr) ((uint32_t)(uint64_t)(ptr))

static inline void *u32_to_ptr(uint32_t x)
{
    uint64_t y = x;
    return (void*) y;
}

static inline uint64_t ptr_to_integer(const void *ptr)
{
    return (uint64_t) ptr;
}

#endif // AARCH64_POINTER_H

