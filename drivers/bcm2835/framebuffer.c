#include <stddef.h>
#include <drivers/mem.h>
#include <memutils.h>

#include "framebuffer.h"
#include "mailbox.h"
#include "dma.h"
#include "pointer.h"

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
            fb->physical_ptr = u32_to_ptr(physical_ptr);
            fb->ptr          = u32_to_ptr((physical_ptr & 0x00ffffff) | CPU_ADDRESS);
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
    uint32_t request[30];
    unaligned_memset(request, 0, sizeof(request));
    /*
        0, // Size of the whole structure (ignore)
        0, // Request
     */

    // TAG 0
    request[2] = 0x48003; // Set width/height
    request[3] = 8; // size
    request[5] = width;
    request[6] = height;

    // TAG 1
    request[7] = 0x48004, // Set virtual width/height (??)
    request[8] = 8; // size
    request[10] = width;
    request[11] = height;

    // TAG 2
    request[12] = 0x48005; // Set depth
    request[13] = 4; // size
    request[15] = 32; // depth

    request[16] = 0x48006; // Set color order
    request[17] = 4; // size
    request[19] = 1; // RGB

    // TAG 3
    request[20] = 0x40008; // Get pitch
    request[21] = 4; // size

    // TAG 4
    request[24] = 0x40001; // Allocate framebuffer (cross your fingers)
    request[25] = 8; // size
    request[27] = 16; // framebuffer alignment

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

