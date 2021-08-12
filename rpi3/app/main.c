#include <stdio.h>

#include "../core/irq.h"

#include <drivers/bcm2835/interrupt.h>
#include <drivers/bcm2835/gpio.h>
#include <drivers/bcm2835/uart.h>
#include <drivers/bcm2835/systimer.h>
#include <drivers/bcm2835/framebuffer.h>
#include <drivers/bcm2835/spi.h>
#include <drivers/bcm2835/clock.h>
#include <drivers/bcm2835/vpu.h>
#include <drivers/virtual/console.h>

#define GPIO_TEST 26
#define WIDTH 800
#define HEIGHT 480

static volatile uint64_t ticks = 0xdeadbeefdeadbeefl;
static volatile uint32_t cpt0 = 0;
static volatile uint32_t cpt1 = 0;

#if 0
static size_t to_hex(size_t x)
{
    x &= 0xf;
    if(x < 10)
        return '0' + x;
    return 'a' + (x - 10);
}

static void put_hex32(char *s, size_t x)
{
    for(size_t i = 8; i--;)
    {
        size_t j = (i << 3);
        *s++ = to_hex(x >> j);
    }
}

static void print_clock_info(struct fb_info_t *fb, uint32_t x, uint32_t y)
{
    char tmp[16];

    struct clock_data_t clock_data;
    if(clock_dump(&clock_data) != 0)
    {
        console_print(fb, x, y, "Clock error");
        return;
    }

    for(size_t cid = FIRST_CLOCK; cid <= LAST_CLOCK; ++cid)
    {
        // 1. Name
        tmp[0] = to_hex(cid);
        tmp[1] = 0;
        console_print(fb, x, y++, tmp);

        // 2. State
        tmp[0] = 'S';
        tmp[1] = 't';
        tmp[2] = 'a';
        tmp[3] = 't';
        tmp[4] = 'e';
        tmp[5] = ':';
        tmp[6] = ' ';
        put_hex32(tmp+7, clock_data.state[cid - FIRST_CLOCK]);
        tmp[15] = 0;
        console_print(fb, x+1, y++, tmp);

        // 3. Rate
        tmp[0] = 'R';
        tmp[1] = 'a';
        tmp[2] = 't';
        tmp[3] = 'e';
        tmp[4] = ':';
        tmp[5] = ' ';
        put_hex32(tmp+6, clock_data.rate[cid - FIRST_CLOCK]);
        tmp[14] = 0;
        console_print(fb, x+1, y++, tmp);
    }
}
#endif

static void screen_demo(void)
{
    struct fb_info_t fb;
    size_t padding;

    uart_print_hex(fb_init(&fb, 800, 480));

    extern uint8_t test_bin_contents[];
    uint32_t ret;
    int error = vpu_execute_code_with_stack(256, test_bin_contents, (uint32_t) fb.physical_ptr, fb.pitch, WIDTH, HEIGHT, 0, &ret);

    padding = (fb.pitch >> 2) - fb.width;

    for(size_t frame = 0; ; ++frame)
    {
        uint32_t *ptr = fb.tmp;
        for(uint32_t y = 0; y < 480; ++y)
        {
            uint8_t Y = (y * 255) / 480 - frame;
            uint32_t color = ((Y << 16) | ((frame & 0xff) << 8) | 0xff);
            for(uint32_t x = 0; x < 640; ++x)
            {
                uint8_t X = (x * 255) / 640 - frame;
                color |= (X << 24);
                *ptr++ = color;
            }
            ptr += padding;
        }
        if(error != 0)
        {
            console_print(&fb, 1, 1, "VPU error");
            console_printhex(&fb, 1, 2, error);
        }

        fb_copy_buffer(&fb);
    }
    for(;;);
}

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

    uart_print("Hello world!\r\n");

    //IRQ_ENABLE();

    screen_demo();

    for(;;)
    {
        systimer_wait_us(1);

        /*
        uart_print("Ticks =\r\n");
        uart_print_hex(ticks);
        uart_print_hex(cpt0);
        uart_print_hex(cpt1);
        */
    }
}

void main1(void)
{
    /* This code is going to be run on core 1 */
    for(;;)
    {
        //ticks = systimer_getticks();
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
