#include "ads8661.h"
#include "spi.h"
#include "uart.h"

union command_u
{
    uint8_t buf[4];
    uint32_t u32;
};

uint32_t ads8661_init(void)
{
    // SCLK = 20MHz, shorten the cables if you want higher clock values
    uint32_t status = spi_init(2000000u, SPI_CS_ACTIVE_LOW, SPI_CPOL0_CPHA0);
    if(status)
        return status;

    ads8661_write(0x0c, 0x0200);
    ads8661_write(0x10, 0x0100); // release=0x100. debug=0x107 (ads8661_sense() should return 0x333)
    ads8661_write(0x14, 0x000b);

    return status;
}

void ads8661_write(uint16_t address, uint16_t value)
{
    address &= 0x01fc;

    union command_u payload = {.buf = {0xd0 | (address >> 8), address, value >> 8, value}};
    union command_u ret;

    spi_rw_buffer(&payload.buf, &ret.buf, sizeof(payload));
}

uint16_t ads8661_read(uint16_t address)
{
    address &= 0x01fc;

    union command_u payload = {.buf = {0xc8 | (address >> 8), address, 0x00, 0x00}};
    union command_u payload_nop = {.u32 = 0x00000000};
    union command_u ret;

    spi_rw_buffer(&payload.buf, &ret.buf, sizeof(payload));
    spi_rw_buffer(&payload_nop.buf, &ret.buf, sizeof(payload_nop));

    // Invert endianness
    return ((uint16_t)ret.buf[0] << 8) | ret.buf[1];
}

uint32_t ads8661_sense(void)
{
    union command_u payload_nop = {.u32 = 0x00000000};
    union command_u ret;

    spi_rw_buffer(&payload_nop.buf, &ret.buf, sizeof(payload_nop));
    spi_rw_buffer(&payload_nop.buf, &ret.buf, sizeof(payload_nop));

    return ret.u32;
}

void ads8661_stream_blocking(uint32_t *buf, size_t maxlen)
{
    union command_u payload_nop = {.u32 = 0x00000000};
    union command_u tmp;

    spi_rw_buffer(&payload_nop.buf, &tmp.buf, sizeof(payload_nop));
    while(maxlen--)
        spi_rw_buffer(&payload_nop.buf, buf++, sizeof(payload_nop));
}

void ads8661_dump(uint32_t *buf)
{
    for(size_t i = 0x00; i <= 0x14; i += 4)
        *buf++ = ads8661_read(i);

    for(size_t i = 0x20; i <= 0x28; i += 4)
        *buf++ = ads8661_read(i);
}
