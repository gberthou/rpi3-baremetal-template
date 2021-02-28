#include <drivers/common.h>
#include "spi.h"
#include "gpio.h"
#include "dma.h"

#define BASE_SPI_FREQ_HZ 250000000u

#define CS_TXD      (1 << 18)
#define CS_RXD      (1 << 17)
#define CS_DONE     (1 << 16)
#define CS_REN      (1 << 12)
#define CS_ADCS     (1 << 11)
#define CS_DMAEN    (1 << 8)
#define CS_TA       (1 << 7)
#define CS_CLEAR_RX (1<<5)
#define CS_CLEAR_TX (1<<4)

#define GPIO_CE0   8
#define GPIO_MOSI0 10
#define GPIO_MISO0 9
#define GPIO_SCLK0 11

/*
ALT4
#define GPIO_CE1   7
#define GPIO_MOSI1 20
#define GPIO_MISO1 19
#define GPIO_SCLK1 21
*/

#define SPI_CS   ((volatile uint32_t*) (PERIPHERAL_BASE + 0x00204000))
#define SPI_FIFO ((volatile uint32_t*) (PERIPHERAL_BASE + 0x00204004))
#define SPI_CLK  ((volatile uint32_t*) (PERIPHERAL_BASE + 0x00204008))

static uint32_t cs_config_nodma = 0;

uint32_t spi_init(uint32_t desiredFreq, enum spi_cs_mode_e csmode, enum spi_data_mode_e datamode)
{
    // SPI cannot operate for frequencies higher than BASE/2
    if(desiredFreq > BASE_SPI_FREQ_HZ / 2)
        return 1;

    // Configure gpios for slave 0
    gpio_select_function(GPIO_CE0, GPIO_ALT0);
    gpio_select_function(GPIO_MOSI0, GPIO_ALT0);
    gpio_select_function(GPIO_MISO0, GPIO_ALT0);
    gpio_select_function(GPIO_SCLK0, GPIO_ALT0);

    // Hardware automatically rounds down clock divider to fit hardware
    // requirements
    *SPI_CLK = BASE_SPI_FREQ_HZ / desiredFreq;

    cs_config_nodma = (csmode << 6) | (datamode << 2);
    *SPI_CS = cs_config_nodma;

    dma_enable(0, 1);
    dma_enable(1, 1);

    return 0;
}

void spi_rw_buffer(const void *rbuffer, void *wbuffer, size_t size)
{
    const uint8_t *_rbuffer = rbuffer;
    uint8_t *_wbuffer = wbuffer;

    *SPI_CS = cs_config_nodma | CS_TA | CS_CLEAR_RX | CS_CLEAR_TX;
    for(size_t i = size; i--;)
    {
        while(!(*SPI_CS & CS_TXD));
        *SPI_FIFO = *_rbuffer++;
    }

    while(size--)
    {
        while(!(*SPI_CS & CS_RXD));
        *_wbuffer++ = *SPI_FIFO;
    }

    while(!(*SPI_CS & CS_DONE));
    *SPI_CS = cs_config_nodma;
}

static struct dma_block_t __attribute__((aligned(256))) block_ram2spi = {
    .transfer_info = (1 << 8) // SRC_INC
                   ,
    .src_addr = 0,
    .dst_addr = PERIPH_TO_PHYS((uint32_t) SPI_FIFO),
    .transfer_len = 0,
    .stride = 0,
    .next = 0
};

static struct dma_block_t __attribute__((aligned(256))) block_spi2ram = {
    .transfer_info = (SPI_RX << 16) // PERMAP
                   | (1 << 10) // SRC_DREQ
                   | (1 << 4) // DST_INC
                   ,
    .src_addr = PERIPH_TO_PHYS((uint32_t) SPI_FIFO),
    .dst_addr = 0,
    .transfer_len = 0,
    .stride = 0,
    .next = 0
};

void spi_rw_dma32(const uint32_t *rbuffer, uint32_t *wbuffer, size_t size_bytes)
{
    size_t size_words = (size_bytes >> 2);

    uint32_t dma_payload[] = {
        (sizeof(uint32_t) << 16) | (cs_config_nodma & 0xff) | CS_TA,
        0
    };

    block_ram2spi.src_addr = VIRT_TO_PHYS((uint32_t) dma_payload);
    block_ram2spi.transfer_len = sizeof(dma_payload);
    block_spi2ram.transfer_len = sizeof(uint32_t);

    *SPI_CS = cs_config_nodma | CS_DMAEN | CS_ADCS | CS_CLEAR_RX | CS_CLEAR_TX;
    while(size_words--)
    {
        dma_payload[1] = *rbuffer;
        block_spi2ram.dst_addr = VIRT_TO_PHYS((uint32_t) (wbuffer++));

        dma_run_async(0, &block_ram2spi);
        dma_run_async(1, &block_spi2ram);
        dma_wait_transfer_done(1);
    }
    *SPI_CS = cs_config_nodma;
}

void spi_rw_dma32_nonblocking(const uint32_t *rbuffer, uint32_t *wbuffer, size_t size_bytes, volatile size_t *ready_bytes, volatile const bool *stop)
{
    size_t size_words = (size_bytes >> 2);
    size_t ready = 0;

    uint32_t dma_payload[] = {
        (sizeof(uint32_t) << 16) | (cs_config_nodma & 0xff) | CS_TA,
        0
    };

    block_ram2spi.src_addr = VIRT_TO_PHYS((uint32_t) dma_payload);
    block_ram2spi.transfer_len = sizeof(dma_payload);
    block_spi2ram.transfer_len = sizeof(uint32_t);

    *SPI_CS = cs_config_nodma | CS_DMAEN | CS_ADCS | CS_CLEAR_RX | CS_CLEAR_TX;
    while(size_words-- && (!stop || !*stop))
    {
        dma_payload[1] = *rbuffer;
        block_spi2ram.dst_addr = VIRT_TO_PHYS((uint32_t) (wbuffer++));

        dma_run_async(0, &block_ram2spi);
        dma_run_async(1, &block_spi2ram);
        dma_wait_transfer_done(1);

        ready += sizeof(uint32_t);
        *ready_bytes = ready;
    }
    *SPI_CS = cs_config_nodma;
}

uint32_t spi_read16_bidirectional(void)
{
    *SPI_CS |= CS_REN | CS_TA;

    *SPI_FIFO = 0xff; // Dummy write to request read from slave
    *SPI_FIFO = 0xff; // Dummy write to request read from slave

    while(!(*SPI_CS & CS_DONE));
    *SPI_CS &= ~CS_TA;

    register uint32_t upper = (*SPI_FIFO << 8);
    register uint32_t lower = *SPI_FIFO;

    return upper | lower;
}

