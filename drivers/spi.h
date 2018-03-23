#ifndef DRIVERS_SPI_H
#define DRIVERS_SPI_H

#include <sys/types.h>

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

/* spi_read_bidirectional
 * 1 <= bytecount <= 4
 * SCLK is interrupted after each transferred byte, by software operation
 * Slave chip is kept enabled during the whole transaction though
 *        _  _  _  _  _  _  _  _        _  _  _  _  _  _  _  _
 * SCLK _/ _/ _/ _/ _/ _/ _/ _/ _______/ _/ _/ _/ _/ _/ _/ _/ _
 */
uint32_t spi_read_bidirectional(size_t bytecount);

#endif

