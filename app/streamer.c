#include <stdio.h>
#include <stdbool.h>

#include "../drivers/uart.h"
#include "../drivers/gpio.h"
#include "../drivers/systimer.h"
#include "../drivers/ads8661.h"

#include "calibration.h"

#define BUFSIZE (1<<13)

#define GPIO_DUT 26 // Pin used by the device under test to notify the monitor
#define GPIO_MON 16 // Pin used by the monitor (RPI) to notidy the device under test

struct measurement_t
{
    // Truncate ticks to 32bit => overflows at 4295s
    uint32_t tick_before;
    uint32_t tick_after;
    uint32_t tick_start;
    uint32_t tick_stop;
    uint32_t data[BUFSIZE];
};

struct display_t
{
    uint32_t tick_before;
    uint32_t tick_after;
    uint32_t tick_start;
    uint32_t tick_stop;
    uint16_t data[BUFSIZE];
};

static volatile unsigned int started = false;
static volatile bool must_acquire = false;
static struct measurement_t measurement;

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
    struct display_t d = {
        .tick_before = measurement->tick_before,
        .tick_after  = measurement->tick_after,
        .tick_start  = measurement->tick_start,
        .tick_stop   = measurement->tick_stop
    };
    for(size_t i = 0; i < length; ++i)
        d.data[i] = measurement->data[i]; // Keep only the last 16bits, which correspond to the most-significant bits since endianness is reversed

    print_raw32_custom64(&d, length*sizeof(uint16_t)+2*sizeof(uint32_t));
}

void streamer_acquisition_thread(void)
{
    started = true;
    for(;;)
    {
        while(!must_acquire);

        acquire(&measurement, BUFSIZE);

        must_acquire = false; // Release
    }
}

void streamer_gpio_thread(void)
{
    while(!started);

    gpio_select_function(GPIO_DUT, GPIO_INPUT);
    gpio_select_function(GPIO_MON, GPIO_OUTPUT);
    gpio_set_resistor(GPIO_DUT, GPIO_RESISTOR_PULLUP);
    gpio_out(GPIO_MON, 0);

    for(;;)
    {
        for(size_t i = 0; i < 2; ++i)
        {
            uint64_t tstop = -1ul;

            while(!gpio_in(GPIO_DUT)); // Wait until DUT is ready
            if(i == 0) // Start
                uart_print("Start\n");
            else // Stop
            {
                tstop = systimer_getticks();
                uart_print("Stop\n");
            }

            gpio_out(GPIO_MON, 1); // Prepare DUT
            while(gpio_in(GPIO_DUT)); // Wait until DUT is ready
            
            if(i == 0) // Start measurement
            {
                must_acquire = true; // Notify acquisition thread
                measurement.tick_start = systimer_getticks();
            }
            else // Stop measurement
            {
                while(must_acquire); // Wait until acquisition is done
                measurement.tick_stop = tstop;
                display(&measurement, BUFSIZE);
            }
            gpio_out(GPIO_MON, 0); // Notify DUT to resume
        }
    }
}

