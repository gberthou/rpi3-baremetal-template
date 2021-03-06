#ifndef DRIVERS_CONSOLE_H
#define DRIVERS_CONSOLE_H

#include <stdint.h>
#include <drivers/bcm2835/framebuffer.h>

void console_print(struct fb_info_t *fb, uint32_t px, uint32_t py, const char *str);
void console_printhex(struct fb_info_t *fb, uint32_t px, uint32_t py, uint32_t x);

#endif

