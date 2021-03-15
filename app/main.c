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

void __attribute__((__naked__)) IRQHandler(void)
{
    __asm__ __volatile__("push {r0-r5, r12, lr}");

    gpio_ack_interrupt(GPIO_TEST);

    __asm__ __volatile__("pop {r0-r5, r12, lr}\r\n"
                         "subs pc, lr, #4");
}

static void init_gpio(void)
{
    gpio_select_function(GPIO_TEST, GPIO_INPUT);
    gpio_set_resistor(GPIO_TEST, GPIO_RESISTOR_PULLUP);
    gpio_set_async_edge_detect(GPIO_TEST, GPIO_FALLING_EDGE, 1);

    interrupt_enable(INT_SOURCE_GPIO);
}

void main0(void)
{
    /* This code is going to be run on core 0 */
    init_gpio();
    uart_init_1415();

    //IRQ_ENABLE();

    uint8_t command[4];
    *midi_note_on(command, 0, MIDI_NOTE_TO_KEY(MIDI_NOTE_A, 0), 64) = 0;
    uart_print((char*)command);

    for(;;)
    {
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
