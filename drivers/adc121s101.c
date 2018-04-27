#include "adc121s101.h"
#include "spi.h"

#define SCLK_FREQ_HZ 2000000

// Datasheet
// http://www.ti.com/lit/ds/symlink/adc121s101.pdf

uint32_t adc_init(void)
{
    // CPOL = 1
    // CPHA = 1
    return spi_init(SCLK_FREQ_HZ, SPI_CS_ACTIVE_LOW, SPI_CPOL1_CPHA1);
}

uint32_t adc_read(void)
{
    return spi_read_bidirectional(2);
}

