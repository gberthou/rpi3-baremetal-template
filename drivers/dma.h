#ifndef DMA_H
#define DMA_H

#include <stddef.h>

struct dma_block_t
{
    uint32_t transfer_info;
    uint32_t src_addr;
    uint32_t dst_addr;
    uint32_t transfer_len;
    uint32_t stride;
    uint32_t next;
};

enum dma_permap_e
{
    SPI_TX = 6,
    SPI_RX
};

void dma_enable(size_t channel, size_t value);
void dma_wait_transfer_done(size_t channel);

// [!] block must be multiple of 256
void dma_run_async(size_t channel, const struct dma_block_t *block);

void dma_memcpy32(uint32_t *dst, const uint32_t *src, size_t size_bytes);

#endif

