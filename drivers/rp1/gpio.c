#include <stdint.h>

#include <platform.h>
#include <drivers/rp1/rp1.h>
#include <drivers/gpio.h>

#define GPIO_BASE (RP1_BASE + 0xd0000)
#define PADS_BASE (RP1_BASE + 0xf0000)

#define PADS_BANK0_GPIOn ((volatile uint32_t*) (PADS_BASE + 0x4))

struct gpio_t
{
    uint32_t status;
    uint32_t ctrl;
};

#define GPIOS ((volatile struct gpio_t*) GPIO_BASE)

void gpio_select_function(unsigned int gpio, enum gpio_function_e function)
{
    volatile uint32_t *ctrl = &GPIOS[gpio].ctrl;
    *RP1_ATOMIC_CLR(ctrl) = (0x3 << 16) // INOVER
                          | (0x3 << 14) // OEOVER
                          | (0x3 << 12) // OUTOVER
                          | 0x1f // FUNCSEL
                          ;
    *RP1_ATOMIC_SET(ctrl) = (function & 0x1f);

    // The usage is unknown but it can be assumed that input and output can be
    // used.
    volatile uint32_t *reg = &PADS_BANK0_GPIOn[gpio];
    *RP1_ATOMIC_CLR(reg) = (1 << 7); // OD
    *RP1_ATOMIC_SET(reg) = (1 << 6); // IE
}

void gpio_set_resistor(unsigned int gpio, enum gpio_resistor_e resistor)
{
    volatile uint32_t *reg = &PADS_BANK0_GPIOn[gpio];
    *RP1_ATOMIC_CLR(reg) = 0xc;
    *RP1_ATOMIC_SET(reg) = (resistor & 0x3) << 2;
}
