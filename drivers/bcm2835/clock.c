#include <stddef.h>
#include <drivers/common.h>

#include "clock.h"
#include "mailbox.h"

#define CLOCK_ARM 3

/* Please see
 * https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
 */

int clock_max_out_arm(void)
{
    uint32_t max_clock_rate = 0;
    volatile uint32_t __attribute__((aligned(16))) sequence0[] = {
        0, // Size of the whole structure
        0, // Request

        0x00030004, // Get max clock rate
        8, // Size
        0, // Request
        CLOCK_ARM,
        0,

        0
    };
    volatile const uint32_t *ptr = sequence0 + 2;
    sequence0[0] = sizeof(sequence0);

    mailbox_send(8, VIDEOBUS_OFFSET + ((uint32_t)sequence0));
    if(mailbox_receive(8) != 0 && sequence0[1] != 0x80000000)
        return 1;
    while(*ptr)
    {
        switch(*ptr++)
        {
            case 0x00030004: // Get max clock rate
                max_clock_rate = ptr[2];
                break;
            
            default: // Panic
                return 1;
        }
        ptr += *ptr / 4 + 2;
    }

    volatile uint32_t __attribute__((aligned(16))) sequence1[] = {
        0, // Size of the whole structure
        0, // Request

        0x00038002, // Get max clock rate
        12, // Size
        0, // Request
        CLOCK_ARM,
        max_clock_rate,
        0,

        0
    };
    sequence1[0] = sizeof(sequence1);
    mailbox_send(8, VIDEOBUS_OFFSET + ((uint32_t)sequence1));
    if(mailbox_receive(8) != 0 && sequence1[1] != 0x80000000)
        return 1;
    return 0;
}

int clock_dump(struct clock_data_t *data)
{
    volatile uint32_t __attribute__((aligned(16))) sequence[3 + 10 * N_CLOCKS] = {
        0, // size of the whole structure
        0  // request
    };

    volatile uint32_t *ptr = sequence + 2;

    // Get clock state
    for(uint32_t cid = FIRST_CLOCK; cid <= LAST_CLOCK; ++cid)
    {
        *ptr++ = 0x00030001; // Get clock state
        *ptr++ = 8; // size
        *ptr++ = 0; // request
        *ptr++ = cid;
        *ptr++ = 0xdeadbeef;
    }

    // Get clock rate
    for(uint32_t cid = FIRST_CLOCK; cid <= LAST_CLOCK; ++cid)
    {
        *ptr++ = 0x00030002; // Get clock rate
        *ptr++ = 8; // size
        *ptr++ = 0; // request
        *ptr++ = cid;
        *ptr++ = 0xdeadbeef;
    }

    *ptr = 0; // End TAG
    sequence[0] = sizeof(sequence);
    
    // Send the requested values
    mailbox_send(8, VIDEOBUS_OFFSET + ((uint32_t)sequence));
    
    // Now get the real values
    if(mailbox_receive(8) != 0 && sequence[1] != 0x80000000)
        return 1;

    {
        volatile const uint32_t *ptr = sequence + 2;
        while(*ptr)
        {
            switch(*ptr++)
            {
                case 0x00030001: // Get clock state
                {
                    uint32_t cid = ptr[2];
                    data->state[cid - FIRST_CLOCK] = ptr[3];
                    break;
                }
                
                case 0x00030002: // Get clock rate
                {
                    uint32_t cid = ptr[2];
                    data->rate[cid - FIRST_CLOCK] = ptr[3];
                    break;
                }

                default: // Panic
                    return 1;
            }
            ptr += *ptr / 4 + 2;
        }
    }

    // Now, set clocks
    volatile uint32_t __attribute__((aligned(16))) sequence1[] = {
        0, // size of the whole structure
        0, // request

        0x00038002, // Set clock rate
        12, // Size
        0, // Request
        3, // Clock ID
        1000000000, // Rate (Hz)
        0, // Don't skip turbo settings

        /*

        0x00038002, // Set clock rate
        12, // Size
        0, // Request
        4, // Clock ID
        1000000000, // Rate (Hz)
        0, // Don't skip turbo settings

        0x00038002, // Set clock rate
        12, // Size
        0, // Request
        5, // Clock ID
        1000000000, // Rate (Hz)
        0, // Don't skip turbo settings

        0x00038002, // Set clock rate
        12, // Size
        0, // Request
        6, // Clock ID
        1000000000, // Rate (Hz)
        0, // Don't skip turbo settings
        */

        0 // End tag
    };
    sequence1[0] = sizeof(sequence1);

    mailbox_send(8, VIDEOBUS_OFFSET + ((uint32_t)sequence1));
    if(mailbox_receive(8) != 0 && sequence1[1] != 0x80000000)
        return 1;

    return 0;
}
