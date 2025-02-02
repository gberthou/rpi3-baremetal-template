#ifndef DRIVERS_BCM2835_DMA_H
#define DRIVERS_BCM2835_DMA_H

#include <stddef.h>

struct dma_dmalite_block_t
{
    uint32_t transfer_info;
    uint32_t src_addr;
    uint32_t dst_addr;
    uint32_t transfer_len;
    uint32_t stride; // DMA only
    uint32_t next;
    uint32_t _pad[2];
};

struct dma4_block_t
{
    uint32_t transfer_info;
    uint32_t src_addr;
    uint32_t src_info;
    uint32_t dst_addr;
    uint32_t dst_info;
    uint32_t transfer_len;
    uint32_t next;
    uint32_t _pad;
};

enum dma_permap_e
{
    SPI_TX = 6,
    SPI_RX
};

void dma_enable(size_t channel, size_t value);
void dma_wait_transfer_done(size_t channel);

void dma_run_async(size_t channel, const void *block);

void dma_memcpy32(uint32_t *dst, const uint32_t *src, size_t size_bytes);

#endif // DRIVERS_BCM2835_DMA_H

