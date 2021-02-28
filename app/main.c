#include <stdio.h>

#include "../core/irq.h"

#include <drivers/bcm2835/interrupt.h>
#include <drivers/bcm2835/gpio.h>
#include <drivers/bcm2835/uart.h>
#include <drivers/bcm2835/systimer.h>
#include <drivers/bcm2835/framebuffer.h>
#include <drivers/bcm2835/spi.h>

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

static void screen_demo(void)
{
    struct fb_info_t fb;

    uart_print_hex(fb_init(&fb, 640, 480));

    for(uint32_t y = 0; y < 480; ++y)
        for(uint32_t x = 0; x < 640; ++x)
        {
            uint8_t X = (x * 255) / 640;
            uint8_t Y = (y * 255) / 480;

            uint32_t color = 0x01000000 * X + 0x00010000 * Y + 0xff;
            fb_put_color(&fb, x, y, color);
        }
}

static void init_gpio(void)
{
    gpio_select_function(GPIO_TEST, GPIO_INPUT);
    gpio_set_resistor(GPIO_TEST, GPIO_RESISTOR_PULLUP);
    gpio_set_async_edge_detect(GPIO_TEST, GPIO_FALLING_EDGE, 1);

    interrupt_enable(INT_SOURCE_GPIO);
}

static void wait_us(uint32_t us)
{
   uint64_t tend = systimer_getticks() + us;
   while(systimer_getticks() < tend);
}

void main0(void)
{
    /* This code is going to be run on core 0 */
    init_gpio();
    uart_init_1415();

    uart_print("Hello world!\r\n");

    //IRQ_ENABLE();

    screen_demo();

    for(;;)
    {
        wait_us(1);

        uart_print("Ticks =\r\n");
        uart_print_hex(ticks);
        uart_print_hex(cpt0);
        uart_print_hex(cpt1);
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
        //--cpt1;
    }
}
