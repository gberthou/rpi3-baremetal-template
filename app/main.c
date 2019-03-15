#include <stdio.h>

#include "../drivers/uart.h"
#include "../drivers/systimer.h"
#include "../drivers/ads8661.h"

#include "calibration.h"
#include "streamer.h"

void main0(void)
{
    /* This code is going to be run on core 0 */

    uart_init_1415();

    uart_print("#### Powerprobe ####\r\n");

    if(ads8661_init() != 0)
    {
        uart_print("ADS8661 init failed\r\n");
        for(;;);
    }

    for(;;)
    {
        uart_print("c: Calibrate\r\n"
                   "s: Stream\r\n");

        uint8_t c = uart_getc();
        while(c != 'c' && c != 's')
            c = uart_getc();

        if(c == 'c')
            calibrate();
        else if(c == 's')
        {
            streamer_acquisition_thread();
        }
    }
}

void main1(void)
{
    /* This code is going to be run on core 1 */

    streamer_display_thread();

    for(;;)
    {
    }
}

void main2(void)
{
    /* This code is going to be run on core 2 */

    streamer_timer_thread();

    for(;;)
    {
    }
}

void main3(void)
{
    /* This code is going to be run on core 3 */

    for(;;)
    {
    }
}
