#include <stdio.h>

#include "../drivers/uart.h"
#include "../drivers/systimer.h"
#include "../drivers/ads8661.h"

#include "calibration.h"

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

static volatile uint64_t ticks = 0;
static volatile size_t measurement_index = 0;
static volatile size_t display_index = 0;
static struct measurement_t measurements[POOLSIZE];

static void acquire(struct measurement_t *measurement, size_t length)
{
    measurement->tick_before = systimer_getticks();
    ads8661_stream_blocking(measurement->data, length);
    measurement->tick_after = systimer_getticks();
}

static void print_raw32_custom64(const void *data, size_t size)
{
    const uint32_t *_data = data;
    size >>= 2;

    while(size--)
    {
        uint32_t value = *_data++;
        for(size_t i = 6, j = 0; i-- > 0; j += 6)
            uart_putc(32 + ((value >> j) & 0x3f));
    }
    uart_putc('\r');
    uart_putc('\n');
}

static void display(const struct measurement_t *measurement, size_t length)
{
    struct display_t d = {.tick_before = measurement->tick_before, .tick_after = measurement->tick_after};
    for(size_t i = 0; i < length; ++i)
        d.data[i] = measurement->data[i]; // Keep only the last 16bits, which correspond to the most-significant bits since endianness is reversed

    print_raw32_custom64(&d, length*sizeof(uint16_t)+2*sizeof(uint32_t));
}

void streamer_acquisition_thread(void)
{
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

void streamer_display_thread(void)
{
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

void streamer_timer_thread(void)
{
    for(;;)
    {
        ticks = systimer_getticks();
    }
}
