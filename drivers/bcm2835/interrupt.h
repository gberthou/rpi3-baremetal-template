#ifndef DRIVERS_INTERRUPT_H
#define DRIVERS_INTERRUPT_H

enum interrupt_source_e
{
    INT_SOURCE_GPIO = 49
};

void interrupt_enable(enum interrupt_source_e source);
void interrupt_disable(enum interrupt_source_e source);

#endif

