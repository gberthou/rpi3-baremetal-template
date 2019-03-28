#include "../drivers/uart.h"
#include "../drivers/ads8661.h"

#define BUFSIZE (1 << 12)

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
    return sum >> 12;
}

void calibrate(void)
{
    static uint32_t buf[BUFSIZE];

    for(;;)
    {
        uart_print("n: New value\r\n"
                   "q: Return\r\n");

        for(;;)
        {
            uint8_t c = uart_getc();
            if(c == 'q')
                return;
            else if(c == 'n')
            {
                ads8661_stream_blocking(buf, BUFSIZE);
                uart_print_hex(mean(buf));
                uart_print("\r\n");
                break;
            }
        }
    }
}

