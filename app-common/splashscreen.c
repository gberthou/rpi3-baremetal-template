#include <stdio.h>

#include "../core/irq.h"

#include <drivers/gpio.h>
#include <drivers/bcm2835/interrupt.h>
#include <drivers/bcm2835/uart.h>
#include <drivers/bcm2835/systimer.h>
#include <drivers/bcm2835/framebuffer.h>
#include <drivers/bcm2835/spi.h>
#include <drivers/bcm2835/vpu.h>
#include <drivers/virtual/console.h>

#define GPIO_TEST 26
#define WIDTH 800
#define HEIGHT 480

void app_screen_demo(void)
{
    struct fb_info_t fb;

    uart_print_hex(fb_init(&fb, 800, 480));

#ifdef VC4_SUPPORT
    extern uint8_t vc4_bin_contents[];
    uint32_t ret;
    int error = vpu_execute_code_with_stack(256, vc4_bin_contents, (uint32_t) fb.physical_ptr, fb.pitch, WIDTH, HEIGHT, 0, &ret);
    if(error != 0)
    {
        console_print(&fb, 1, 1, "VPU error");
        console_printhex(&fb, 1, 2, error);
    }

    fb_copy_buffer(&fb);
#else
    size_t padding = (fb.pitch >> 2) - fb.width;
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

        fb_copy_buffer(&fb);
    }
#endif
    for(;;);
}

void app_init_gpio(void)
{
    gpio_select_function(GPIO_TEST,
#if RPI < 5
        GPIO_INPUT
#else
        GPIO_PIO
#endif
    );
    gpio_set_resistor(GPIO_TEST, GPIO_RESISTOR_PULLUP);
    //gpio_set_async_edge_detect(GPIO_TEST, GPIO_FALLING_EDGE, 1);

    interrupt_enable(INT_SOURCE_GPIO);
}

