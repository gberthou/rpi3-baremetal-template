#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <sys/types.h>

// Format: ARGB?

struct FBInfo
{
    uint32_t *ptr;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
};

int FBInit(struct FBInfo *fb, uint32_t width, uint32_t height);
void FBPutColor(struct FBInfo *fb, uint32_t x, uint32_t y, uint32_t color);

#endif

