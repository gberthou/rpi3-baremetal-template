#include <stddef.h>
#include <drivers/common.h>

#include "clock.h"
#include "mailbox.h"

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

bool clock_max_out_arm(void)
{
    const uint32_t request_get[] = {
        0, // Size of the whole structure (ignore)
        0, // Request

        0x00030004, // Get max clock rate
        8, // Size
        0, // Request
        CLOCK_ARM,
        0,

        0
    }; 
    uint32_t max_clock_rate = 0;

    if(!mailbox_request(request_get, sizeof(request_get), clock_max_out_arm_get_cb, &max_clock_rate))
        return 1;

    const uint32_t request_set[] = {
        0, // Size of the whole structure
        0, // Request

        0x00038002, // Set clock rate
        12, // Size
        0, // Request
        CLOCK_ARM,
        max_clock_rate,
        0,

        0
    };

    if(!mailbox_request(request_set, sizeof(request_set), mailbox_ack, NULL))
        return 1;
    return 0;
}

