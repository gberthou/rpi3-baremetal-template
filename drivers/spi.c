#include "spi.h"
#include "common.h"
#include "gpio.h"

const uint32_t BASE_SPI_FREQ_HZ = 250000000;

const uint32_t CS_DONE = (1 << 16);
const uint32_t CS_REN = (1 << 12);
const uint32_t CS_TA = (1 << 7);

#if 0
#else

// CE0  = gpio8 -> alt0
// MOSI = gpio10 -> alt0
// MISO = gpio9 -> alt0 
// SCLK = gpio11 -> alt0

const unsigned int GPIO_CE0 = 8;
const unsigned int GPIO_MOSI0 = 10;
const unsigned int GPIO_MISO0 = 9;
const unsigned int GPIO_SCLK0 = 11;

/*
ALT4
const unsigned int GPIO_CE1 = 7;
const unsigned int GPIO_MOSI1 = 20;
const unsigned int GPIO_MISO1 = 19;
const unsigned int GPIO_SCLK1 = 21;
*/

static volatile uint32_t * const SPI_CS   = (uint32_t*) (PERIPHERAL_BASE + 0x00204000);
static volatile uint32_t * const SPI_FIFO = (uint32_t*) (PERIPHERAL_BASE + 0x00204004);
static volatile uint32_t * const SPI_CLK  = (uint32_t*) (PERIPHERAL_BASE + 0x00204008);
#endif

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

    *SPI_CS = (csmode << 6) | (datamode << 2);

    return 0;
}

uint32_t spi_read_bidirectional(size_t bytecount)
{
    uint32_t tmp = 0;

    *SPI_CS |= CS_REN | CS_TA;

    while(bytecount--)
    {
        *SPI_FIFO = 0; // Dummy write to request read from slave
        while(!(*SPI_CS & CS_DONE));
        tmp = (tmp << 8) | *SPI_FIFO;
    }

    *SPI_CS &= ~CS_TA;
    return tmp;
}

