#ifndef DRIVERS_SPI_H
#define DRIVERS_SPI_H

#include <sys/types.h>

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
uint32_t spi_read_bidirectional(void);

#endif

