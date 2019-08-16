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
static bool summary = false;

static void acquire(struct measurement_t *measurement, size_t length)
{
    measurement->tick_before = systimer_getticks();
    ads8661_stream_blocking(measurement->data, length);
    measurement->tick_after = systimer_getticks();
}

static void print_raw32_custom64(const void *data, size_t size)
{
    const uint32_t *_data = data;
    if((size%4) != 0)
    {
        uart_print("print_raw32_custom64: size should be multiple of 4\r\n");
        for(;;);
    }

    size >>= 2;

    while(size--)
    {
        uint32_t value = *_data++;
        for(size_t j = 0; j < 32; j += 6)
            uart_putc(32 + ((value >> j) & 0x3f));
    }
    uart_putc('\r');
    uart_putc('\n');
}

static void display(const struct measurement_t *measurement, size_t length)
{
    static struct display_t d;
    d.tick_before = measurement->tick_before;
    d.tick_after  = measurement->tick_after;
    d.tick_start  = measurement->tick_start;
    d.tick_stop   = measurement->tick_stop;
    for(size_t i = 0; i < length; ++i)
        d.data[i] = measurement->data[i]; // Keep only the last 16bits, which correspond to the most-significant bits since endianness is reversed

    print_raw32_custom64(&d, sizeof(d));
}

static uint32_t reverse_endianness_and_extract(uint32_t x)
{
    return ((((x & 0xff) << 4) | ((x >> 12) & 0xf)) & 0xfff);
}

static uint32_t reverse_endianness_and_sum(const uint32_t *data, size_t length)
{
    uint32_t sum = 0;
    while(length--)
    {
        uint32_t old_sum = sum;
        sum += reverse_endianness_and_extract(*data++);
        if(old_sum > sum) // Overflow
            uart_print("[Error] reverse_endianness_and_sum: overflow\r\n");
    }
    return sum;
}

// length >= 1
static uint32_t max_adc(const uint32_t *data, size_t length)
{
    uint32_t max = reverse_endianness_and_extract(*data++);
    --length;
    while(length--)
    {
        uint32_t tmp = reverse_endianness_and_extract(*data++);
        if(tmp > max)
            max = tmp;
    }
    return max;
}

static void display_summary(const struct measurement_t *measurement)
{
    size_t total_dt = measurement->tick_after - measurement->tick_before;
    size_t duration = measurement->tick_stop - measurement->tick_start;


    // For now, offset is always 0
    //size_t offset = (measurement->tick_start - measurement->tick_before) * BUSIZE / total_dt;
    size_t offset = 0;
    size_t length = duration * BUFSIZE / total_dt;

    uint32_t sum = reverse_endianness_and_sum(measurement->data + offset, length);
    uint32_t m_adc = max_adc(measurement->data + offset, length);

    uart_print("buf_size   : ");
    uart_print_hex(BUFSIZE);
    uart_print("\r\n");

    uart_print("sum_adc    : ");
    uart_print_hex(sum);
    uart_print("\r\n");

    uart_print("total_dt_us: ");
    uart_print_hex(total_dt);
    uart_print("\r\n");

    uart_print("duration_us: ");
    uart_print_hex(duration);
    uart_print("\r\n");

    uart_print("max_adc    : ");
    uart_print_hex(m_adc);
    uart_print("\r\n\r\n");
}

void streamer_acquisition_thread(bool s)
{
    summary = s;
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
    gpio_set_resistor(GPIO_DUT, GPIO_RESISTOR_PULLDOWN);
    gpio_out(GPIO_MON, 0);

    for(;;)
    {
        for(size_t i = 0; i < 2; ++i)
        {
            uint64_t tstop = -1ul;

            while(!gpio_in(GPIO_DUT)); // Wait until DUT is ready
            if(i == 0) // Start
            {
                if(!summary)
                    uart_print("Start\r\n");
            }
            else // Stop
            {
                tstop = systimer_getticks();
                if(!summary)
                    uart_print("Stop\r\n");
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
                if(summary)
                    display_summary(&measurement);
                else
                    display(&measurement, BUFSIZE);
            }
            gpio_out(GPIO_MON, 0); // Notify DUT to resume
        }
    }
}

