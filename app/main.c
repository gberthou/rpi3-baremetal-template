#include <stdio.h>

#include "../core/irq.h"

#include <drivers/bcm2835/interrupt.h>
#include <drivers/bcm2835/gpio.h>
#include <drivers/bcm2835/uart.h>
#include <drivers/bcm2835/systimer.h>
#include <drivers/bcm2835/framebuffer.h>
#include <drivers/bcm2835/spi.h>
#include <drivers/bcm2835/clock.h>
#include <drivers/bcm2835/vpu.h>
#include <drivers/virtual/console.h>
#include <drivers/virtual/midi.h>

#define GPIO_TEST 26

#define GPIO_BTN1 0
#define GPIO_BTN2 1
#define GPIO_BTN3 5
#define GPIO_BTN4 6
#define GPIO_BTN5 12
#define GPIO_BTN6 16

#define OUT_MIN 0x0666
#define OUT_MAX 0x399a
#define P_MIN_PA 0
#define P_MAX_PA 206843
#define PA_2_OUT(p) ((int32_t)(OUT_MIN + ((p) - P_MIN_PA) * ((float)(OUT_MAX - OUT_MIN)) / (P_MAX_PA - P_MIN_PA)))

#define OUT_BREATH_THRESHOLD_OCT1 PA_2_OUT(2000)
#define OUT_BREATH_THRESHOLD_OCT2 PA_2_OUT(4000)

#define OCTAVE_OFFSET 4

const size_t BUTTONS[] = {GPIO_BTN1, GPIO_BTN2, GPIO_BTN3, GPIO_BTN4, GPIO_BTN5, GPIO_BTN6};

static bool isnew(uint16_t x)
{
    return (x & 0xc000) == 0x0000;
}

// Assumes that all buttons are < 32
static uint8_t aggregate_buttons(void)
{
    uint32_t gpio = gpio_array_in(0);
    uint8_t ret = 0;

    for(size_t i = sizeof(BUTTONS)/sizeof(BUTTONS[0]); i--;)
        if(gpio & (1u << BUTTONS[i]))
            ret |= (1u << i);

    return ret;
}

static int buttons_to_note(uint8_t buttons)
{
    switch(buttons)
    {
        case 0x00:
            return MIDI_NOTE_A;
        case 0x01:
            return MIDI_NOTE_G;
        case 0x02:
            return MIDI_NOTE_GS;
        case 0x03:
            return MIDI_NOTE_F;
        case 0x07:
            return MIDI_NOTE_DS;
        case 0x0f:
            return MIDI_NOTE_D;
        case 0x1f:
            return MIDI_NOTE_C;
        case 0x3f:
            return MIDI_NOTE_AS;
        default:
            return -1;
    }
}

static int note_to_key(int note, unsigned int octave)
{
    return MIDI_NOTE_TO_KEY(note, OCTAVE_OFFSET -1 + octave - (note == MIDI_NOTE_AS ? 1 : 0));
}

void __attribute__((__naked__)) IRQHandler(void)
{
    __asm__ __volatile__("push {r0-r5, r12, lr}");

    gpio_ack_interrupt(GPIO_TEST);

    __asm__ __volatile__("pop {r0-r5, r12, lr}\r\n"
                         "subs pc, lr, #4");
}

static void init_gpio(void)
{
#if 0
    gpio_select_function(GPIO_TEST, GPIO_INPUT);
    gpio_set_resistor(GPIO_TEST, GPIO_RESISTOR_PULLUP);
    gpio_set_async_edge_detect(GPIO_TEST, GPIO_FALLING_EDGE, 1);

    interrupt_enable(INT_SOURCE_GPIO);
#else
    gpio_select_function(GPIO_BTN1, GPIO_INPUT);
    gpio_select_function(GPIO_BTN2, GPIO_INPUT);
    gpio_select_function(GPIO_BTN3, GPIO_INPUT);
    gpio_select_function(GPIO_BTN4, GPIO_INPUT);
    gpio_select_function(GPIO_BTN5, GPIO_INPUT);
    gpio_select_function(GPIO_BTN6, GPIO_INPUT);

    gpio_set_resistor(GPIO_BTN1, GPIO_RESISTOR_PULLDOWN);
    gpio_set_resistor(GPIO_BTN2, GPIO_RESISTOR_PULLDOWN);
    gpio_set_resistor(GPIO_BTN3, GPIO_RESISTOR_PULLDOWN);
    gpio_set_resistor(GPIO_BTN4, GPIO_RESISTOR_PULLDOWN);
    gpio_set_resistor(GPIO_BTN5, GPIO_RESISTOR_PULLDOWN);
    gpio_set_resistor(GPIO_BTN6, GPIO_RESISTOR_PULLDOWN);
#endif
}

void main0(void)
{
    /* This code is going to be run on core 0 */
    uint16_t rdata, wdata;
    init_gpio();
    spi_init(750000u, SPI_CS_ACTIVE_LOW, SPI_CPOL0_CPHA0);
    uart_init_1415();

    //IRQ_ENABLE();

    uint8_t command[7];

    int last_key = 128;

    for(;;)
    {
        spi_rw_buffer(&rdata, &wdata, sizeof(rdata));
        uint16_t ordered = (((wdata & 0xff) << 8) | (wdata >> 8));
        if(isnew(ordered))
        {
            uint16_t _value = (ordered & 0xfff);
            int16_t value = _value;

            if(value >= OUT_BREATH_THRESHOLD_OCT1)
            {
                unsigned int octave = (value >= OUT_BREATH_THRESHOLD_OCT2 ? 2 : 1);
                uint8_t buttons = aggregate_buttons();
                int note = buttons_to_note(buttons);

                if(note > 0)
                {
                    int key = note_to_key(note, octave);

                    if(key != last_key)
                    {
                        if(last_key > 0)
                        {
                            *midi_note_off(command, 0, last_key, 64) = 0;
                            uart_print((char*)command);
                        }
                        *midi_note_on(command, 0, key, 64) = 0;
                        uart_print((char*)command);

                        last_key = key;
                    }
                }
            }
            else if(last_key > 0)
            {
                *midi_note_off(command, 0, last_key, 64) = 0;
                uart_print((char*)command);

                last_key = -1;
            }
        }
        systimer_wait_us(10000);
    }
}

void main1(void)
{
    /* This code is going to be run on core 1 */
    for(;;)
    {
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
    }
}
