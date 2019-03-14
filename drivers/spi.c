#include "spi.h"
#include "common.h"
#include "gpio.h"

#define BASE_SPI_FREQ_HZ 250000000u

#define CS_TXD (1 << 18)
#define CS_RXD (1 << 17)
#define CS_DONE (1 << 16)
#define CS_REN  (1 << 12)
#define CS_TA   (1 << 7)
#define CS_CLEAR_RX (1<<5)
#define CS_CLEAR_TX (1<<4)

// CE0  = gpio8 -> alt0
// MOSI = gpio10 -> alt0
// MISO = gpio9 -> alt0 
// SCLK = gpio11 -> alt0

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

static uint32_t cs_config = 0;

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

    cs_config = (csmode << 6) | (datamode << 2);
    *SPI_CS = cs_config;

    return 0;
}

void spi_rw_buffer(const void *rbuffer, void *wbuffer, size_t size)
{
    const uint8_t *_rbuffer = rbuffer;
    uint8_t *_wbuffer = wbuffer;

    *SPI_CS = cs_config | CS_TA | CS_CLEAR_RX | CS_CLEAR_TX;
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
    *SPI_CS = cs_config;
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

