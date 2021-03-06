#ifndef DRIVERS_FRAMEBUFFER_H
#define DRIVERS_FRAMEBUFFER_H

#include <stdint.h>

// Format: ARGB?

struct fb_info_t
{
    volatile uint32_t *ptr;
    void *physical_ptr;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    size_t ptr_size;

    uint32_t *tmp;
};

int fb_init(struct fb_info_t *fb, uint32_t width, uint32_t height);
void fb_put_color(struct fb_info_t *fb, uint32_t x, uint32_t y, uint32_t color);
void fb_copy_buffer(struct fb_info_t *fb);

#define fb_put_color_tmp(fb, x, y, color) ((fb)->tmp[(x) + (y) * ((fb)->pitch >> 2)] = (color))

#endif

