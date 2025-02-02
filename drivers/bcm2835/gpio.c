#include <stdint.h>
#include <stddef.h>

#include <drivers/common.h>
#include <drivers/gpio.h>
#include <platform.h>

#if RPI < 5
#define GPIO_BASE (PERIPHERAL_BASE + 0x00200000)
#define GPFSEL0   ((volatile uint32_t*) (GPIO_BASE + 0x00))
#define GPSET0    ((volatile uint32_t*) (GPIO_BASE + 0x1c))
#define GPCLR0    ((volatile uint32_t*) (GPIO_BASE + 0x28))
#define GPLEV0    ((volatile uint32_t*) (GPIO_BASE + 0x34))
#define GPPUD     ((volatile uint32_t*) (GPIO_BASE + 0x94))
#define GPPUDCLK0 ((volatile uint32_t*) (GPIO_BASE + 0x98))
#define GPAREN0   ((volatile uint32_t*) (GPIO_BASE + 0x7c))
#define GPAFEN0   ((volatile uint32_t*) (GPIO_BASE + 0x88))
#define GPEDS0    ((volatile uint32_t*) (GPIO_BASE + 0x40))
#else
#define GPIO_BASE (RP1_BASE + 0xd0000)
#define PADS_BASE (RP1_BASE + 0xf0000)
#endif

void gpio_select_function(unsigned int gpio, enum gpio_function_e function)
{
    size_t offset = gpio / 10;
    size_t shift  = 3 * (gpio % 10);

    uint32_t tmp = GPFSEL0[offset];
    tmp &= ~(0x7u << shift);
    tmp |= ((function & 0x7) << shift);
    GPFSEL0[offset] = tmp;
}

void gpio_set_resistor(unsigned int gpio, enum gpio_resistor_e resistor)
{
    unsigned int tmp;
    size_t offset = (gpio > 31 ? 1 : 0);
    size_t mask   = 1u << (gpio % 32);

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
    size_t offset = (gpio > 31 ? 1 : 0);
    size_t mask   = 1u << (gpio % 32);

    if(value)
        GPSET0[offset] = mask;
    else
        GPCLR0[offset] = mask;
}

unsigned int gpio_in(unsigned int gpio)
{
    size_t offset = (gpio > 31 ? 1 : 0);
    size_t mask   = 1u << (gpio % 32);

    return (GPLEV0[offset] & mask) != 0;
}

void gpio_set_async_edge_detect(unsigned int gpio, enum gpio_edge_e edge, unsigned int enable)
{
    size_t offset = (gpio > 31 ? 1 : 0);
    size_t mask   = 1u << (gpio % 32);

    if(enable)
    {
        if(edge == GPIO_FALLING_EDGE)
            GPAFEN0[offset] |= mask;
        else // GPIO_RISING_EDGE
            GPAREN0[offset] |= mask;
    }
    else
    {
        if(edge == GPIO_FALLING_EDGE)
            GPAFEN0[offset] &= ~mask;
        else // GPIO_RISING_EDGE
            GPAREN0[offset] &= ~mask;
    }
}

void gpio_ack_interrupt(unsigned int gpio)
{
    size_t offset = (gpio > 31 ? 1 : 0);
    size_t mask   = 1u << (gpio % 32);

    GPEDS0[offset] = mask;
}
