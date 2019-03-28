#ifndef DRIVERS_SPI_H
#define DRIVERS_SPI_H

#include <stdint.h>
#include <stddef.h>

/* This minimalistic driver only supports one SPI slave, to be wired to the spi_[ce|sclk|miso|mosi]_0 pins.
 */

enum spi_cs_mode_e
{
    SPI_CS_ACTIVE_LOW = 0,
    SPI_CS_ACTIVE_HIGH = 1
};

enum spi_data_mode_e
{
    SPI_CPOL0_CPHA0 = 0,
    SPI_CPOL0_CPHA1 = 1,
    SPI_CPOL1_CPHA0 = 2,
    SPI_CPOL1_CPHA1 = 3
};

uint32_t spi_init(uint32_t desiredFreq, enum spi_cs_mode_e csmode, enum spi_data_mode_e datamode);

void spi_rw_buffer(const void *rbuffer, void *wbuffer, size_t size);

void spi_rw_dma32(const uint32_t *rbuffer, uint32_t *wbuffer, size_t size_bytes);

/* spi_read16_bidirectional
 * Reads 2 bytes from spi slabe.
 * SCLK is interrupted after each transferred byte, by software operation
 * Slave chip is kept enabled during the whole transaction though
 *        _  _  _  _  _  _  _  _        _  _  _  _  _  _  _  _
 * SCLK _/ _/ _/ _/ _/ _/ _/ _/ _______/ _/ _/ _/ _/ _/ _/ _/ _
 */
uint32_t spi_read16_bidirectional(void);

#endif

