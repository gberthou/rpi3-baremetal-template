#include <stdint.h>
#include <stddef.h>

#define WIDTH 800
#define HEIGHT 480

void kmain(uint32_t r0, uint32_t r1)
{
    volatile uint32_t *framebuffer = (volatile uint32_t*) r0;
    size_t pitch = r1;

    for(size_t frame = 0; ; ++frame)
    {
        volatile uint32_t *ptr = framebuffer;

        for(uint32_t y = 0; y < HEIGHT; ++y)
        {
            uint8_t Y = (y * 255) / HEIGHT - frame;
            uint32_t color = ((Y << 16) | ((frame & 0xff) << 8) | 0xff);

            volatile uint32_t *tmp = ptr;
            for(uint32_t x = 0; x < WIDTH; ++x)
            {
                uint8_t X = (x * 255) / WIDTH - frame;
                color |= (X << 24);
                *tmp++ = color;
            }
            ptr += (pitch >> 2);
        }
    }
}

