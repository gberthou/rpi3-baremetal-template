#include <stdint.h>
#include <stddef.h>

#include <drivers/common.h>
#include <utils.h>
#include <platform.h>
#include "dma.h"

struct dma_chan_t
{
    uint32_t cs;
    uint32_t block_addr;
    uint32_t transfer_info;
    uint32_t src_addr;
    uint32_t dst_addr;
    uint32_t transfer_len;
    uint32_t stride;
    uint32_t next;
    uint32_t debug;
};

#define DMA_BASE       ((volatile uint32_t*) (PERIPHERAL_BASE + 0x00007000))
#define DMA_INT_STATUS ((volatile uint32_t*) (PERIPHERAL_BASE + 0x00007fe0))
#define DMA_ENABLE     ((volatile uint32_t*) (PERIPHERAL_BASE + 0x00007ff0))
#define DMAX(x)   ((volatile struct dma_chan_t*) (DMA_BASE + (x) * 0x40))

#define DMA_CHAN_MEMCPY 0

void dma_enable(size_t channel, size_t value)
{
    if(value)
        *DMA_ENABLE |= (1u << channel);
    else
        *DMA_ENABLE &= ~(1u << channel);
}

void dma_wait_transfer_done(size_t channel)
{
    volatile struct dma_chan_t *ptr = DMAX(channel);
    while((ptr->cs & 0x1));
    ptr->cs = 0x80000000;
    memoryBarrier();
}

void dma_run_async(size_t channel, const struct dma_block_t *block)
{
    volatile struct dma_chan_t *ptr = DMAX(channel);
    ptr->cs = 0x80000000; // Reset dma channel
    ptr->block_addr = VIRT_TO_PHYS((uint32_t) block);
    ptr->cs = 0x00000001; // Activate DMA
}

void dma_memcpy32(uint32_t *dst, const uint32_t *src, size_t size_bytes)
{
    static __attribute__((aligned(256))) struct dma_block_t block;
    block.transfer_info = (1 << 8) // SRC_INC
                        | (1 << 4) // DST_INC
                        ;
    block.src_addr = VIRT_TO_PHYS((uint32_t) src);
    block.dst_addr = VIRT_TO_PHYS((uint32_t) dst);
    block.transfer_len = size_bytes;
    block.stride = 0;
    block.next = 0;

    dma_run_async(DMA_CHAN_MEMCPY, &block);
    dma_wait_transfer_done(DMA_CHAN_MEMCPY);
}

void dma_memcpy32_physical_dst(uint32_t *dst, const uint32_t *src, size_t size_bytes)
{
    static __attribute__((aligned(256))) struct dma_block_t block;
    block.transfer_info = (1 << 8) // SRC_INC
                        | (1 << 4) // DST_INC
                        ;
    block.src_addr = VIRT_TO_PHYS((uint32_t) src);
    block.dst_addr = (uint32_t) dst;
    block.transfer_len = size_bytes;
    block.stride = 0;
    block.next = 0;

    dma_run_async(DMA_CHAN_MEMCPY, &block);
    dma_wait_transfer_done(DMA_CHAN_MEMCPY);
}

