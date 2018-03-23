#include <stdio.h>

#include "../drivers/gpio.h"
#include "../drivers/uart.h"
#include "../drivers/systimer.h"
#include "../drivers/framebuffer.h"
#include "../drivers/spi.h"

#define CPUFREQ 800000000ul

static void u64_to_hex(char *dst, uint64_t x)
{
    static const char FIGURES[] = "0123456789abcdef";

    for(unsigned int i = 0; i < 16; ++i)
    {
        dst[15 - i] = FIGURES[x & 0xf];
        x >>= 4;
    }
}

static void print_hex(uint64_t x)
{
    char tmp[19];
    tmp[16] = '\r';
    tmp[17] = '\n';
    tmp[18] = 0;

    u64_to_hex(tmp, x);
    uart_print(tmp);
}

void main0(void)
{
    /* This code is going to be run on core 0 */

    uart_init_1415();

    uart_print("Hello world!\r\n");
#if 0

    struct FBInfo fb;

    uart_print("FBInit:\r\n");
    int ret = FBInit(&fb, 600, 480);
    print_hex(ret);

    uart_print("Done!\r\n");

    for(uint32_t y = 0; y < 480; ++y)
        for(uint32_t x = 0; x < 600; ++x)
            FBPutColor(&fb, x, y, 0xff00ffff);

    for(;;);
#else
    uart_print("spi_init\r\n");
    spi_init(1000000, SPI_CS_ACTIVE_LOW, SPI_CPOL0_CPHA0);

    for(;;)
    {
        uart_print("Value =\r\n");
        print_hex(
        spi_read_bidirectional()
        );
    }
#endif
}

void main1(void)
{
    /* This code is going to be run on core 1 */
}

void main2(void)
{
    /* This code is going to be run on core 2 */
}

void main3(void)
{
    /* This code is going to be run on core 3 */
}
