#include "framebuffer.h"
#include "mailbox.h"

static const uint32_t VIDEOBUS_OFFSET = 0x80000000u;

/* Please see
 * https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
 */
int fb_init(struct fb_info_t *fb, uint32_t width, uint32_t height)
{
    volatile uint32_t __attribute__((aligned(16))) sequence[] = {
        0, // size of the whole structure
        0, // request
        
        // TAG 0
        0x48003, // Set width/height
        8, // size
        8, // request
        width,
        height,

        // TAG 1
        0x48004, // Set virtual width/height (??)
        8, // size
        8, // request
        width,
        height,

        // TAG 2
        0x48005, // Set depth
        4, // size
        4, // request
        32, // depth

        // TAG 3
        0x40008, // Get pitch
        4, // size
        4, //request
        0,

        // TAG 4
        0x40001, // Allocate framebuffer (cross your fingers)
        8, // size
        8, // request
        16, // framebuffer alignment
        0,

        0 // End TAG
    };
    sequence[0] = sizeof(sequence);
    

    // Send the requested values
    mailbox_send(8, VIDEOBUS_OFFSET + ((uint32_t)sequence));
    
    // Now get the real values
    if(mailbox_receive(8) == 0 || sequence[1] == 0x80000000) // Ok
    {
        volatile const uint32_t *ptr = sequence + 2;
        while(*ptr)
        {
            switch(*ptr++)
            {
                case 0x40008:
                    fb->pitch = ptr[2];
                    break;

                case 0x48003:
                    fb->width = ptr[2];
                    fb->height = ptr[3];
                    break;

                case 0x40001:
                    fb->ptr = (uint32_t*) ((ptr[2] & ~VIDEOBUS_OFFSET) | 0x3f000000);
                    break;

                case 0x48004:
                case 0x48005:
                    break;

                default: // Panic
                    return 1;
            }
            ptr += *ptr / 4 + 2;
        }
        return 0;
    }
    return 1;
}

void fb_put_color(struct fb_info_t *fb, uint32_t x, uint32_t y, uint32_t color)
{
    fb->ptr[x + y * (fb->pitch >> 2)] = color;
}

