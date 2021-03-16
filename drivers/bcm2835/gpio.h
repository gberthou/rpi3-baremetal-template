#ifndef DRIVERS_GPIO_H
#define DRIVERS_GPIO_H

enum gpio_function_e
{
    GPIO_INPUT,
    GPIO_OUTPUT,
    GPIO_ALT5,
    GPIO_ALT4,
    GPIO_ALT0,
    GPIO_ALT1,
    GPIO_ALT2,
    GPIO_ALT3
};

enum gpio_resistor_e
{
    GPIO_RESISTOR_NONE,
    GPIO_RESISTOR_PULLDOWN,
    GPIO_RESISTOR_PULLUP
};

enum gpio_edge_e
{
    GPIO_FALLING_EDGE,
    GPIO_RISING_EDGE
};

void gpio_select_function(unsigned int gpio, enum gpio_function_e function);
void gpio_set_resistor(unsigned int gpio, enum gpio_resistor_e resistor);
void gpio_out(unsigned int gpio, unsigned int value);
unsigned int gpio_in(unsigned int gpio);
uint32_t gpio_array_in(size_t index);
void gpio_set_async_edge_detect(unsigned int gpio, enum gpio_edge_e edge, unsigned int enable);
void gpio_ack_interrupt(unsigned int gpio);

#endif

