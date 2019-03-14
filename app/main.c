#include <stdio.h>

#include "../drivers/uart.h"
#include "../drivers/systimer.h"
#include "../drivers/ads8661.h"

#define BUFSIZE (1<<12)
#define POOLSIZE 32

struct measurement_t
{
    uint64_t tick_before;
    uint64_t tick_after;
    uint32_t data[BUFSIZE];
};

static volatile uint64_t ticks = 0xdeadbeefdeadbeefl;
static volatile size_t measurement_index = 0;
static volatile size_t display_index = 0;
static struct measurement_t measurements[POOLSIZE];

static void wait_us(uint32_t us)
{
   uint64_t tend = systimer_getticks() + us;
   while(systimer_getticks() < tend);
}

void acquire(struct measurement_t *measurement, size_t length)
{
    measurement->tick_before = systimer_getticks();
    ads8661_stream_blocking(measurement->data, length);
    measurement->tick_after = systimer_getticks();
}

void display(const struct measurement_t *measurement, size_t length)
{
    uart_print_raw32(measurement, length*sizeof(uint32_t)+2*sizeof(uint64_t));
}

void main0(void)
{
    /* This code is going to be run on core 0 */

    uart_init_1415();

    uart_print("Hello world!\r\n");

    if(ads8661_init() != 0)
    {
        uart_print("Failed\r\n");
        for(;;);
    }

    wait_us(50000);

    for(;;)
    {
        if(measurement_index >= POOLSIZE)
        {
            // Synchronize with display thread
            while(display_index < POOLSIZE);
            measurement_index = 0;
        }

        acquire(measurements + measurement_index, BUFSIZE);
        ++measurement_index;
    }
}

void main1(void)
{
    /* This code is going to be run on core 1 */

    for(;;)
    {
        if(display_index >= POOLSIZE)
        {
            // Synchronize with acquisition thread
            while(measurement_index >= POOLSIZE);
            display_index = 0;
        }
        while(measurement_index <= display_index);

        display(measurements + display_index, BUFSIZE);
        ++display_index;
    }
}

void main2(void)
{
    /* This code is going to be run on core 2 */

    for(;;)
    {
    }
}

void main3(void)
{
    /* This code is going to be run on core 3 */

    for(;;)
    {
        ticks = systimer_getticks();
    }
}
