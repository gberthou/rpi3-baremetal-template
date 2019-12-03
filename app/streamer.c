#include <stdio.h>
#include <stdbool.h>

#include "../drivers/uart.h"
#include "../drivers/gpio.h"
#include "../drivers/systimer.h"
#include "../drivers/ads8661.h"

#include "calibration.h"
#include "clock.h"

#define BUFSIZE (1<<16)
#define NONBLOCKING
#define FASTCLOCK

#define GPIO_DUT 26 // Pin used by the device under test to notify the monitor
#define GPIO_MON 16 // Pin used by the monitor (RPI) to notidy the device under test

#ifdef FASTCLOCK
#define GETTICKS() (latest_clock)
#else
#define GETTICKS() (systimer_getticks())
#endif

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
static volatile bool printing = false;
static volatile size_t ready_bytes;
static volatile bool stop;
static struct measurement_t measurement;
static bool summary = false;

static void acquire(struct measurement_t *measurement, size_t length)
{
    ready_bytes = 0;
    stop = false;
    
    measurement->tick_before = GETTICKS();
#ifdef NONBLOCKING
    ads8661_stream_nonblocking(measurement->data, length, &ready_bytes, &stop);
#else
    ads8661_stream_blocking(measurement->data, length);
#endif
    measurement->tick_after = GETTICKS();
}

#ifndef NONBLOCKING
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
    uart_putc('\r');
    uart_putc('\n');
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

    uart_print("sum_adc    : ");
    uart_print_hex(sum);

    uart_print("total_dt_us: ");
    uart_print_hex(total_dt);

    uart_print("duration_us: ");
    uart_print_hex(duration);

    uart_print("max_adc    : ");
    uart_print_hex(m_adc);
    uart_print("\r\n");
}
#endif

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
            // In non-blocking mode, tstop is irrelevant
#ifndef NONBLOCKING
            uint64_t tstop = -1ul;
#endif

            while(!gpio_in(GPIO_DUT)); // Wait until DUT is ready
#ifndef NONBLOCKING
            if(i == 0) // Start
            {
                if(!summary)
                    uart_print("Start\r\n");
            }
            else // Stop
            {
                tstop = GETTICKS();
                if(!summary)
                    uart_print("Stop\r\n");
            }
#endif


            gpio_out(GPIO_MON, 1); // Prepare DUT
            while(gpio_in(GPIO_DUT)); // Wait until DUT is ready
            
            if(i == 0) // Start measurement
            {
                must_acquire = true; // Notify acquisition thread
                measurement.tick_start = GETTICKS();
            }
            else // Stop measurement
            {
                stop = true;
                while(must_acquire); // Wait until acquisition is done
#ifndef NONBLOCKING
                measurement.tick_stop = tstop;
                if(summary)
                    display_summary(&measurement);
                else
                    display(&measurement, BUFSIZE);
#else
                while(printing);
#endif
            }
            gpio_out(GPIO_MON, 0); // Notify DUT to resume
        }
    }
}

#ifdef NONBLOCKING
static void u12_to_custom64(uint16_t u12, char *ret)
{
    ret[1] = ' ' + (u12 & 0x3f);
    ret[0] = ' ' + ((u12 >> 6) & 0x3f);
}
#endif

void streamer_display_thread(void)
{
#ifdef NONBLOCKING
    while(!started);
    printing = true;

    for(;;)
    {
        size_t printed_bytes = 0;
        while(!must_acquire);

        uart_print("StreamNB\r\n");
        while(!stop || printed_bytes < ready_bytes)
        {
            while(printed_bytes + sizeof(uint32_t) >= ready_bytes);
            size_t offset_words = (printed_bytes >> 2);
            size_t to_print_words = ((ready_bytes-printed_bytes) >> 2);
            char buf[2];
            for(size_t i = 0; i < to_print_words; ++i)
            {
                // data: XX XX CX AB
                // Cast to uint16_t => CX AB
                uint16_t u12 = measurement.data[offset_words + i]; 
                // Reverse endianness and right shift => A BC
                u12 = (((u12 & 0xff) << 4) | (u12 >> 12));
                u12_to_custom64(u12, buf);
                uart_putc(buf[0]);
                uart_putc(buf[1]);
            }
            printed_bytes += (to_print_words << 2);
        }
        uart_print("\r\n");
        uart_print_hex(ready_bytes >> 1); // Amount of measures
        while(must_acquire);
        uart_print_hex(measurement.tick_before);
        uart_print_hex(measurement.tick_after);
        uart_print_hex(measurement.tick_start);
        uart_print("\r\n");
        printing = false;
    }
#endif
}
