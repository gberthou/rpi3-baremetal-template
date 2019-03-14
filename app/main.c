#include <stdio.h>

#include "../core/irq.h"

#include "../drivers/interrupt.h"
#include "../drivers/gpio.h"
#include "../drivers/uart.h"
#include "../drivers/systimer.h"
#include "../drivers/framebuffer.h"
#include "../drivers/spi.h"
#include "../drivers/ads8661.h"

static volatile uint64_t ticks = 0xdeadbeefdeadbeefl;

static void wait_us(uint32_t us)
{
   uint64_t tend = systimer_getticks() + us;
   while(systimer_getticks() < tend);
}

void main0(void)
{
    /* This code is going to be run on core 0 */
    uart_init_1415();

    uart_print("Hello world!\r\n");

    if(ads8661_init() != 0)
    {
        uart_print("Failed\r\n");
        for(;;);
    }

    for(;;)
    {
        wait_us(500000);

        uart_print("Ticks = ");
        uart_print_hex(ticks);
        uart_print("\r\n");
    }
}

void main1(void)
{
    /* This code is going to be run on core 1 */
    for(;;)
    {
        ticks = systimer_getticks();
    }
}

void main2(void)
{
    /* This code is going to be run on core 2 */

    for(;;)
    {
    }
}

void main3(void)
{
    /* This code is going to be run on core 3 */
    for(;;)
    {
    }
}
