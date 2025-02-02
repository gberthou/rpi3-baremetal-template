#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <drivers/common.h>
#include <memutils.h>

#include "mailbox.h"

#define FIRST_CLOCK 0x1
#define LAST_CLOCK  0xe
#define N_CLOCKS (LAST_CLOCK - FIRST_CLOCK + 1)

#define CLOCK_ARM 3

/* Please see
 * https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
 */

static bool clock_max_out_arm_get_cb(const uint32_t *message, void *context)
{
    if(message[0] == 0x00030004) // Get max clock rate
    {
        uint32_t *max_clock_rate = context;
        *max_clock_rate = message[3];
        return true;
    }
    return false;
}

int clock_max_out_arm(void)
{
    uint32_t request[9];
    /* Structure:
        0, // Size of the whole structure (ignore)
        0, // Request

        0x00030004, // Get max clock rate
        8, // Size
        0, // Request
        CLOCK_ARM,
        0,

        0
    */
    unaligned_memset(request, 0, sizeof(request));
    request[2] = 0x00030004; // Get max clock rate
    request[3] = 8; // Size
    request[5] = CLOCK_ARM;

    uint32_t max_clock_rate = 0;
    // The get command only uses 8 words
    if(!mailbox_request(request, 8 * sizeof(uint32_t), clock_max_out_arm_get_cb, &max_clock_rate))
        return 1;

    unaligned_memset(request, 0, sizeof(request));
    request[2] = 0x00038002; // Set clock rate
    request[3] = 12; // Size
    request[5] = CLOCK_ARM;
    request[6] = max_clock_rate;

    if(!mailbox_request(request, sizeof(request), mailbox_ack, NULL))
        return 1;
    return 0;
}

