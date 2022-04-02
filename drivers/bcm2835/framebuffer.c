#include <stddef.h>
#include <drivers/mem.h>

#include "framebuffer.h"
#include "mailbox.h"
#include "dma.h"

#define VIDEOBUS_OFFSET 0x80000000
#define CPU_ADDRESS     0x3e000000

bool fb_init_cb(const uint32_t *message, void *context)
{
    struct fb_info_t *fb = context;
    switch(*message)
    {
        case 0x40008:
            fb->pitch = message[3];
            break;

        case 0x48003:
            fb->width = message[3];
            fb->height = message[4];
            break;

        case 0x40001:
        {
            uint32_t physical_ptr = message[3];
            fb->physical_ptr = (void*) physical_ptr;
            fb->ptr          = (uint32_t*) ((physical_ptr & 0x00ffffff) | CPU_ADDRESS);
            break;
        }

        case 0x48004:
        case 0x48005:
        case 0x48006:
            break;

        default: // Panic
            return false;
    }
    return true;
}

int fb_init(struct fb_info_t *fb, uint32_t width, uint32_t height)
{
    const uint32_t request[] = {
        0, // Size of the whole structure (ignore)
        0, // Request
        
        // TAG 0
        0x48003, // Set width/height
        8, // size
        0, // request
        width,
        height,

        // TAG 1
        0x48004, // Set virtual width/height (??)
        8, // size
        0, // request
        width,
        height,

        // TAG 2
        0x48005, // Set depth
        4, // size
        0, // request
        32, // depth

        0x48006, // Set color order
        4, // size
        0, // request
        1, // RGB

        // TAG 3
        0x40008, // Get pitch
        4, // size
        0, //request
        0,

        // TAG 4
        0x40001, // Allocate framebuffer (cross your fingers)
        8, // size
        0, // request
        16, // framebuffer alignment
        0,

        0 // End TAG
    };
    if(!mailbox_request(request, sizeof(request), fb_init_cb, fb))
        return 1;

    size_t ptr_size = fb->height * fb->pitch;
    void *tmp = malloc(ptr_size);
    if(tmp == NULL)
        return 1;

    fb->ptr_size = ptr_size;
    fb->tmp = tmp;
    return 0;
}

void fb_put_color(struct fb_info_t *fb, uint32_t x, uint32_t y, uint32_t color)
{
    fb->ptr[x + y * (fb->pitch >> 2)] = color;
}

void fb_copy_buffer(struct fb_info_t *fb)
{
    dma_memcpy32_physical_dst(fb->physical_ptr, fb->tmp, fb->ptr_size);
}

