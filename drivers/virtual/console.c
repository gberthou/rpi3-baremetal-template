#include <stddef.h>

#include "console.h"

#define GLYPH_W 8
#define GLYPH_H 8

extern uint8_t GLYPHS[];

static void displayChar(struct fb_info_t *fb, unsigned char c, uint32_t x, uint32_t y)
{
    uint8_t *glyph_data = GLYPHS + c * GLYPH_H;

    size_t pitch = (fb->pitch >> 2);
    uint32_t *ptr = fb->tmp + (x + y * pitch);
    size_t padding = pitch - GLYPH_W;

    for(uint32_t localY = 0; localY < GLYPH_H; ++localY)
    {
        uint8_t data = *glyph_data++;
        uint8_t mask = (1u << (GLYPH_W - 1));
        for(uint32_t localX = 0; localX < GLYPH_W; ++localX, mask >>= 1u)
            *ptr++ = (data & mask) == 0 ? 0xFF000000 : 0xFFFFFFFF;
        ptr += padding;
    } 
}

void console_print(struct fb_info_t *fb, uint32_t px, uint32_t py, const char *str)
{
    uint32_t x = px * GLYPH_W;
    uint32_t y = py * GLYPH_H;

    unsigned char c;
    for(; (c = *str++); x += GLYPH_W)
        displayChar(fb, c, x, y);
}

void console_printhex(struct fb_info_t *fb, uint32_t px, uint32_t py, uint32_t x)
{
    const char figures[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    char s[11];
    uint32_t i;
    uint32_t tmp;

    s[0] = '0';
    s[1] = 'x';
    s[10] = 0;
    for(i = 0; i < 8; ++i)
    {
        tmp = (x >> ((7-i) << 2)) & 0xF;
        s[2 + i] = figures[tmp];
    }    
    
    console_print(fb, px, py, s);
}

