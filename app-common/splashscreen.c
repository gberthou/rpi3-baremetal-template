#include <stddef.h>
#include <sys/types.h>

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
#define HEIGHT 600
#define LOG2_INV_SPEED 6
#define RATIO_WIDTH 23

struct pixel_t
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

// YUV to RGB with Y=0.5, U=x, Y=y
struct pixel_t pixel_at(size_t width_, size_t height_, size_t x, size_t y)
{
    int32_t width = width_;
    int32_t height = height_;
    int32_t U_width = ((int32_t) x) - width / 2;
    int32_t V_height = ((int32_t) y) - height / 2;

    // Small twist: ring at center
    size_t distance2_to_center = U_width * U_width + V_height * V_height;
    if(distance2_to_center >= 100 * 100 && distance2_to_center <= 150 * 150)
    {
        U_width += width - 1 - 2 * x;
        V_height += height - 1 - 2 * y;
    }

    int32_t r_20 = (1 << 19) + (V_height << 20) / height;
    int32_t g_18 = (1 << 17) - (50900 * U_width) / width - (133480 * V_height) / height;
    int32_t b_20 = (1 << 19) + (U_width << 20) / width;

    struct pixel_t pixel = {
        .r = (r_20 >> 12),
        .g = (g_18 >> 10),
        .b = (b_20 >> 12)
    };
    return pixel;
}

static inline uint8_t mix8(uint8_t x1_, uint8_t x2_, uint32_t a)
{
    uint32_t x1 = x1_;
    uint32_t x2 = x2_;
    return (a * x1 + ((1 << RATIO_WIDTH) - a) * x2) >> RATIO_WIDTH;
}

static inline uint32_t rgb(uint32_t r, uint32_t g, uint32_t b)
{
    r &= 0xff;
    g &= 0xff;
    b &= 0xff;

    return (0xff << 24) | (b << 16) | (g << 8) | r;
}

void app_screen_demo(void)
{
    struct fb_info_t fb;

    uart_print_hex(fb_init(&fb, WIDTH, HEIGHT));

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
    int32_t ratio = 0;
    bool up = true;
    for(;;)
    {
        uint32_t *ptr = fb.tmp;
        uint32_t *rptr = fb.tmp + (fb.pitch >> 2) * (fb.height - 1) + fb.width - 1;
        for(uint32_t y = 0; y < fb.height / 2; ++y)
        {
            uint32_t *line = ptr;
            uint32_t *rline = rptr;
            for(uint32_t x = 0; x < fb.width; ++x)
            {
                struct pixel_t px1 = pixel_at(fb.width, fb.height, x, y);
                struct pixel_t px2 = pixel_at(fb.width, fb.height,
                                              fb.width - 1 - x, fb.height - 1 - y);
                uint32_t r1 = mix8(px1.r, px2.r, ratio);
                uint32_t g1 = mix8(px1.g, px2.g, ratio);
                uint32_t b1 = mix8(px1.b, px2.b, ratio);
                uint32_t r2 = mix8(px2.r, px1.r, ratio);
                uint32_t g2 = mix8(px2.g, px1.g, ratio);
                uint32_t b2 = mix8(px2.b, px1.b, ratio);
                uint32_t color1 = rgb(r1, g1, b1);
                uint32_t color2 = rgb(r2, g2, b2);
                *line++ = color1;
                *rline-- = color2;
            }
            ptr += (fb.pitch >> 2);
            rptr -= (fb.pitch >> 2);
        }

        if(up)
            ratio += (1 << (RATIO_WIDTH - LOG2_INV_SPEED));
        else
            ratio -= (1 << (RATIO_WIDTH - LOG2_INV_SPEED));
        if(ratio >> RATIO_WIDTH)
        {
            ratio = (1 << RATIO_WIDTH) - 1;
            up = false;
        }
        else if(ratio < 0)
        {
            ratio = 0;
            up = true;
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

