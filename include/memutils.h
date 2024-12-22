#ifndef MEMUTILS_H
#define MEMUTILS_H

#include <stdint.h>
#include <stddef.h>

static inline void *unaligned_memset(void *_dst, int _c, size_t count)
{
    unsigned char c = _c;
    unsigned char *dst = _dst;
    while(count--)
        *dst++ = c;
    return _dst;
}

static inline void *unaligned_memcpy(void *_dst, const void *_src, size_t count)
{
    unsigned char *dst = _dst;
    const unsigned char *src = _src;
    while(count--)
        *dst++ = *src++;
    return _dst;
}

static inline volatile void *unaligned_vmemcpy(volatile void *_dst, const void *_src, size_t count)
{
    volatile unsigned char *dst = _dst;
    const unsigned char *src = _src;
    while(count--)
        *dst++ = *src++;
    return _dst;
}

#endif // MEMUTILS_H

