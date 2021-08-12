#include <stdint.h>
#include <stddef.h>

#include "interrupt.h"
#include <drivers/common.h>
#include <platform.h>

#define IRQPEN ((volatile uint32_t*)(PERIPHERAL_BASE + 0x0000b204))
#define IRQENA ((volatile uint32_t*)(PERIPHERAL_BASE + 0x0000b210))
#define IRQDIS ((volatile uint32_t*)(PERIPHERAL_BASE + 0x0000b21c))

void interrupt_enable(enum interrupt_source_e source)
{
    size_t offset = (source > 31 ? 1 : 0);
    size_t mask = 1u << (source % 32);

    IRQENA[offset] = mask;
}

void interrupt_disable(enum interrupt_source_e source)
{
    size_t offset = (source > 31 ? 1 : 0);
    size_t mask = 1u << (source % 32);

    IRQDIS[offset] = mask;
}

