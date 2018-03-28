#ifndef DRIVERS_FRAMEBUFFER_H
#define DRIVERS_FRAMEBUFFER_H

#include <sys/types.h>

// Format: ARGB?

struct fb_info_t
{
    volatile uint32_t *ptr;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
};

int fb_init(struct fb_info_t *fb, uint32_t width, uint32_t height);
void fb_put_color(struct fb_info_t *fb, uint32_t x, uint32_t y, uint32_t color);

#endif

