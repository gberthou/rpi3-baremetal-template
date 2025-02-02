#ifndef DRIVERS_GPIO_H
#define DRIVERS_GPIO_H

#if RPI < 5
#include "bcm2835/gpio.h"
#else
#include "rp1/gpio.h"
#endif

enum gpio_resistor_e
{
    GPIO_RESISTOR_NONE,
    GPIO_RESISTOR_PULLDOWN,
    GPIO_RESISTOR_PULLUP
};

void gpio_select_function(unsigned int gpio, enum gpio_function_e function);
void gpio_set_resistor(unsigned int gpio, enum gpio_resistor_e resistor);

#endif // DRIVERS_GPIO_H
