#include <stdio.h>

#include "../drivers/uart.h"
#include "../drivers/systimer.h"
#include "../drivers/ads8661.h"

#define BUFSIZE (1<<12)
#define POOLSIZE 32

struct measurement_t
{
    // Truncate ticks to 32bit => overflows at 4295s
    uint32_t tick_before;
    uint32_t tick_after;
    uint32_t data[BUFSIZE];
};

struct display_t
{
    uint32_t tick_before;
    uint32_t tick_after;
    uint16_t data[BUFSIZE];
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
    struct display_t d = {.tick_before = measurement->tick_before, .tick_after = measurement->tick_after};
    for(size_t i = 0; i < length; ++i)
        d.data[i] = measurement->data[i]; // Keep only the last 16bits, which correspond to the most-significant bits since endianness is reversed

    uart_print_raw32(&d, length*sizeof(uint16_t)+2*sizeof(uint32_t));
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
        for(size_t i = 0; i < POOLSIZE; ++i)
        {
            acquire(measurements + measurement_index, BUFSIZE);
            ++measurement_index;
        }
        
        // Synchronize with display thread
        while(display_index < POOLSIZE);
        measurement_index = 0;
    }
}

void main1(void)
{
    /* This code is going to be run on core 1 */

    for(;;)
    {
        for(size_t i = 0; i < POOLSIZE; ++i)
        {
            while(measurement_index <= display_index);

            display(measurements + display_index, BUFSIZE);
            ++display_index;
        }
        
        // Synchronize with acquisition thread
        while(measurement_index >= POOLSIZE);
        display_index = 0;
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
