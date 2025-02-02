#include <stdint.h>
#include <stddef.h>

#include <drivers/common.h>
#include <utils.h>
#include <platform.h>
#include "dma.h"
#include "pointer.h"

#if __ARM_64BIT_STATE
#include <core/cache64.h>
#else
#include <core/cache32.h>
#endif

struct dma_dmalite_chan_t
{
    uint32_t cs;
    uint32_t block_addr;
    uint32_t transfer_info;
    uint32_t src_addr;
    uint32_t dst_addr;
    uint32_t transfer_len;
    uint32_t stride; // DMA only
    uint32_t next;
    uint32_t debug;
};

struct dma4_chan_t
{
    uint32_t cs;
    uint32_t block_addr;
    uint32_t _pad;
    uint32_t debug;
    uint32_t transfer_info;
    uint32_t src_addr;
    uint32_t src_info;
    uint32_t dst_addr;
    uint32_t dst_info;
    uint32_t transfer_len;
    uint32_t next;
    uint32_t debug2;
};

enum dma_type_e
{
    DMA_TYPE_NORMAL,
    DMA_TYPE_LITE,
#if __ARM_64BIT_STATE
    DMA_TYPE_DMA4
#endif
};

#if RPI < 5
#define DMA_BASE (PERIPHERAL_BASE + 0x7000)
#else
#define DMA_BASE (PERIPHERAL_BASE + 0x10000)
#endif

#define DMA_ENABLE   ((volatile uint32_t*) (DMA_BASE + 0xff0))
#define DMAX_DMA(x)  ((volatile struct dma_dmalite_chan_t*) (DMA_BASE + (x) * 0x100))
#if __ARM_64BIT_STATE
#define DMAX_DMA4(x) ((volatile struct dma4_chan_t*) (DMA_BASE + (x) * 0x100))
#endif

#if RPI < 5
#define DMA4_FIRST_CHAN (~0ul)
#define DMA_CHAN_MEMCPY 0
#else
#define DMA4_FIRST_CHAN 6
#define DMA_CHAN_MEMCPY DMA4_FIRST_CHAN
#endif

// TODO: Several blocks and/or mutexes for concurrent accesses
static __attribute__((aligned(32))) union {
    struct dma_dmalite_block_t dma_dmalite;
    struct dma4_block_t dma4;
} dma_block;

static inline enum dma_type_e dma_type_of_channel(size_t channel)
{
#if __ARM_64BIT_STATE
    if(channel >= DMA4_FIRST_CHAN)
        return DMA_TYPE_DMA4;
#endif
    uint32_t lite = DMAX_DMA(channel)->debug & (1 << 28);
    if(lite)
        return DMA_TYPE_LITE;
    return DMA_TYPE_NORMAL;
}

void dma_enable(size_t channel, size_t value)
{
    if(value)
        *DMA_ENABLE |= (1u << channel);
    else
        *DMA_ENABLE &= ~(1u << channel);
}

static void dma_reset(size_t channel)
{
    // TODO: cache values of dma_type_of_channel(x)
#if __ARM_64BIT_STATE
    if(dma_type_of_channel(channel) == DMA_TYPE_DMA4)
        DMAX_DMA4(channel)->debug |= 0x80000000;
    else
#endif
        DMAX_DMA(channel)->cs = 0x80000000;
}

void dma_wait_transfer_done(size_t channel)
{
    // This part should be portable to DMA/LITE/DMA4 as CS keeps the same offset
    volatile struct dma_dmalite_chan_t *ptr = DMAX_DMA(channel);
    while((ptr->cs & 0x1));
    ptr->cs = 0x80000000;
    memoryBarrier();
}

void dma_run_async(size_t channel, const void *block)
{
    dma_reset(channel);
    memoryBarrier();
    cache_invalidate_range_to_poc(false, ptr_to_integer(block), sizeof(dma_block));

    // DMA4 maps bits [36:5] of the control block address onto bits [31:0] of the CB
    // register
    uint64_t block_address = ptr_to_u32(block);
#if __ARM_64BIT_STATE
    bool is_dma4 = (dma_type_of_channel(channel) == DMA_TYPE_DMA4);
    if(is_dma4)
        block_address >>= 5;
#else
    bool is_dma4 = false;
#endif

    // The struct should be portable to DMA/LITE/DMA4 as CS and CB keep the same offsets
    volatile struct dma_dmalite_chan_t *ptr = DMAX_DMA(channel);
    ptr->block_addr = block_address;
    memoryBarrier();
    ptr->cs = 1 // Active
            | (is_dma4 ? (1 << 28) // Wait for outstanding writes
                       | (0xf << 20) // PANIC_QOS
                       | (1 << 16) // QOS
                       : 0)
            ;
    memoryBarrier();
}

static void *init_block(
    enum dma_type_e channel_type,
    const void *dst,
    const void *src,
    size_t bytes
)
{
#if __ARM_64BIT_STATE
    if(channel_type == DMA_TYPE_DMA4)
    {
        auto src_addr = ptr_to_integer(src);
        auto dst_addr = ptr_to_integer(dst);

        dma_block.dma4.transfer_info = 0;
        dma_block.dma4.src_addr = src_addr; // Bits [31:0]
        dma_block.dma4.src_info = (1 << 12) // INC
                                | ((src_addr >> 32) & 0x1ff)
                                ;
        dma_block.dma4.dst_addr = dst_addr; // Bits [31:0]
        dma_block.dma4.dst_info = (1 << 12) // INC
                                | ((dst_addr >> 32) & 0x1ff)
                                ;
        dma_block.dma4.transfer_len = bytes;
        dma_block.dma4.next = 0;
    }
    else
#else
    (void) channel_type;
#endif
    {
        dma_block.dma_dmalite.transfer_info = (1 << 8) // SRC_INC
                                            | (1 << 4) // DST_INC
                                            ;
        dma_block.dma_dmalite.src_addr = ptr_to_integer(src);
        dma_block.dma_dmalite.dst_addr = ptr_to_integer(dst);
        dma_block.dma_dmalite.transfer_len = bytes; // TODY: might be capped to 0xffff for LITE
        dma_block.dma_dmalite.stride = 0;
        dma_block.dma_dmalite.next = 0;
    }
    return &dma_block;
}

void dma_memcpy32(uint32_t *dst, const uint32_t *src, size_t size_bytes)
{
    enum dma_type_e channel_type = dma_type_of_channel(DMA_CHAN_MEMCPY);
    void *ptr = init_block(channel_type, dst, src, size_bytes);

    cache_invalidate_range_to_poc(false, ptr_to_integer(src), size_bytes);
    dma_enable(DMA_CHAN_MEMCPY, 1);
    dma_run_async(DMA_CHAN_MEMCPY, ptr);
    dma_wait_transfer_done(DMA_CHAN_MEMCPY);
    cache_invalidate_range_to_poc(false, ptr_to_integer(dst), size_bytes);
}

