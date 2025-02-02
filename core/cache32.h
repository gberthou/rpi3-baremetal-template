#ifndef CORE_CACHE32_H
#define CORE_CACHE32_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <utils.h>

static inline size_t cache_line_width_for_poc(bool is_icache)
{
#if RPI == 1
    uint32_t cachetype;
    __asm__("mrc p15, 0, %0, c0, c0, 1" : "=r"(cachetype));
    
    uint32_t shift = (is_icache ? 0 : 12);
    uint32_t linesize = (cachetype >> shift) & 0x3;
#else
    uint32_t clidr;
    __asm__("mrc p15, 1, %0, c0, c0, 1" : "=r"(clidr));
    uint32_t loc = (clidr >> 24) & 0x7;

    __asm__("mcr p15, 2, %0, c0, c0, 0" // CSSELR
            :: "r"((loc << 1) | (is_icache ? 1 : 0))
    );
    isb();

    uint32_t ccsidr;
    __asm__("mrc p15, 1, %0, c0, c0, 0" : "=r"(ccsidr));
    uint32_t linesize = ccsidr & 0x7;
#endif
    return 1ul << (linesize + 4);
}

static inline void cache_invalidate_range_to_poc(bool is_icache, uint32_t begin, size_t size)
{
    size_t linewidth = cache_line_width_for_poc(is_icache);
    size_t linemask = linewidth - 1;
    uint64_t end = (begin + size - 1 + linemask) & ~linemask;
    begin &= ~linemask;
    while(begin < end)
    {
        __asm__("mcr p15, 0, %0, c7, c14, 1" :: "r"(begin)); // DCCIMVAC
        begin += linewidth;
    }
}

#endif // CORE_CACHE32_H
