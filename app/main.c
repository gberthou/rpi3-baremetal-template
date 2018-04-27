#include <stdio.h>

#include "../core/irq.h"

#include "../drivers/interrupt.h"
#include "../drivers/gpio.h"
#include "../drivers/uart.h"
#include "../drivers/systimer.h"
#include "../drivers/framebuffer.h"
#include "../drivers/spi.h"
#include "../drivers/adc121s101.h"

#define GPIO_TEST 26

static volatile uint64_t ticks = 0xdeadbeefdeadbeefl;
static volatile uint32_t cpt0 = 0;
static volatile uint32_t cpt1 = 0;

void __attribute__((__naked__)) IRQHandler(void)
{
    __asm__ __volatile__("push {r0-r5, r12, lr}");

    gpio_ack_interrupt(GPIO_TEST);
    --cpt1;

    __asm__ __volatile__("pop {r0-r5, r12, lr}\r\n"
                         "subs pc, lr, #4");
}

/*
static void screen_demo(void)
{
    struct fb_info_t fb;

    print_hex(fb_init(&fb, 640, 480));

    for(uint32_t y = 0; y < 480; ++y)
        for(uint32_t x = 0; x < 640; ++x)
            fb_put_color(&fb, x, y, 0xff00ffff);
}
*/

static void init_gpio(void)
{
    gpio_select_function(GPIO_TEST, GPIO_INPUT);
    gpio_set_resistor(GPIO_TEST, GPIO_RESISTOR_PULLUP);
    gpio_set_async_edge_detect(GPIO_TEST, GPIO_FALLING_EDGE, 1);

    interrupt_enable(INT_SOURCE_GPIO);
}

void main0(void)
{
    /* This code is going to be run on core 0 */
    init_gpio();
    uart_init_1415();
    adc_init();

    uart_print("Hello world!\r\n");

    IRQ_ENABLE();

    //screen_demo();

#if 0
    uart_print("spi_init\r\n");
    spi_init(1000000, SPI_CS_ACTIVE_LOW, SPI_CPOL0_CPHA0);

    for(;;)
    {
        uart_print("Value =\r\n");
        print_hex(
        spi_read_bidirectional(2)
        );
    }
#else

    for(;;)
    {
        cpt0 = adc_read();
        for(unsigned int i = 0; i < 10000; ++i) __asm__ __volatile__("nop");

        uart_print("Ticks =\r\n");
        uart_print_hex(ticks);
        uart_print_hex(cpt0);
        uart_print_hex(cpt1);
    }
#endif
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
        //--cpt1;
    }
}
