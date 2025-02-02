#ifndef CORE_CACHE64_H
#define CORE_CACHE64_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <utils.h>

static inline size_t cache_line_width_for_poc(bool is_icache)
{
    uint64_t clidr;
    __asm__("mrs %x0, CLIDR_EL1" : "=r"(clidr));
    uint64_t loc = (clidr >> 24) & 0x7;

    __asm__("msr CSSELR_EL1, %x0"
            :: "r"((loc << 1) | (is_icache ? 1 : 0))
    );
    isb();

    uint64_t ccsidr;
    __asm__("mrs %x0, CCSIDR_EL1" : "=r"(ccsidr));
    uint64_t linesize = ccsidr & 0x7;
    return 1ul << (linesize + 4);
}

static inline void cache_invalidate_range_to_poc(bool is_icache, uint64_t begin, size_t size)
{
    size_t linewidth = cache_line_width_for_poc(is_icache);
    size_t linemask = linewidth - 1;
    uint64_t end = (begin + size - 1 + linemask) & ~linemask;
    begin &= ~linemask;
    while(begin < end)
    {
        __asm__("dc civac, %x0" :: "r"(begin));
        begin += linewidth;
    }
}

#endif // CORE_CACHE64_H
