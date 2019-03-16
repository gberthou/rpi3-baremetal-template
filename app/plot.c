#include "../drivers/uart.h"
#include "../drivers/ads8661.h"

#define WIDTH 80

#define BUFSIZE (1 << 10)

static uint32_t acq2value(uint32_t x)
{
    // Reverse endianness
    // AA BX XX XX is encoded XX XX BX AA
    // Must return 00 00 0A AB
    return ((x&0xff) << 4) | ((x>>12)&0xf);
}

// [!] Beware overflow
// TODO: make it less hacky
static uint32_t mean(const uint32_t *buf)
{
    uint32_t sum = 0;
    for(size_t i = BUFSIZE; i--;)
        sum += acq2value(*buf++);
    return sum >> 10;
}

static inline size_t value2pos(uint32_t value)
{
    return (value * WIDTH) >> 12;
}

void plot(void)
{
    uint32_t buf[BUFSIZE];

    for(;;)
    {
        for(size_t i = 1; i < WIDTH; ++i)
            uart_putc('-');
        uart_print(">\r\n");

        for(;;)
        {
            ads8661_stream_blocking(buf, BUFSIZE);
            size_t pos = value2pos(mean(buf));
            for(size_t i = 0; i < pos; ++i)
                uart_putc(' ');
            uart_putc('o');
            for(size_t i = pos+1; i < WIDTH; ++i)
                uart_putc(' ');
            uart_print("\r\n");
        }
    }
}

