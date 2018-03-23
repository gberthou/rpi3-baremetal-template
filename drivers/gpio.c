#include "common.h"
#include "gpio.h"

volatile uint32_t * const GPFSEL0   = (uint32_t*) (PERIPHERAL_BASE + 0x00200000);
volatile uint32_t * const GPSET0    = (uint32_t*) (PERIPHERAL_BASE + 0x0020001C);
volatile uint32_t * const GPCLR0    = (uint32_t*) (PERIPHERAL_BASE + 0x00200028);
volatile uint32_t * const GPPUD     = (uint32_t*) (PERIPHERAL_BASE + 0x00200094);
volatile uint32_t * const GPPUDCLK0 = (uint32_t*) (PERIPHERAL_BASE + 0x00200098);

void gpio_select_function(unsigned int gpio, enum gpio_function_e function)
{
    size_t offset = gpio / 10;
    size_t shift  = 3 * (gpio % 10);

    uint32_t tmp = GPFSEL0[offset];
    tmp &= ~(0x7 << shift);
    tmp |= ((function & 0x7) << shift);
    GPFSEL0[offset] = tmp;
}

void gpio_set_resistor(unsigned int gpio, enum gpio_resistor_e resistor)
{
    unsigned int tmp;
    size_t offset = gpio / 32;
    size_t mask   = 1 << (gpio % 32);

    *GPPUD = resistor;

    for(tmp = 0; tmp < 150; ++tmp)
        __asm__ __volatile__("nop");

    GPPUDCLK0[offset] = mask;

    for(tmp = 0; tmp < 150; ++tmp)
        __asm__ __volatile__("nop");

    GPPUDCLK0[offset] = 0;
}

void gpio_out(unsigned int gpio, unsigned int value)
{
    if(value)
        GPSET0[gpio / 32] = (1 << (gpio % 32));
    else
        GPCLR0[gpio / 32] = (1 << (gpio % 32));
}

