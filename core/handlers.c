#include "irq.h"

INTERRUPT IRQHandler(void)
{
    __asm__ __volatile__("push {r0-r5, r12, lr}");

    //gpio_ack_interrupt(GPIO_TEST);
    //--cpt1;

    __asm__ __volatile__("pop {r0-r5, r12, lr}");
    RETI;
}

INTERRUPT UndefinedHandler(void)
{
    RETI;
}

INTERRUPT SwiHandler(void)
{
    RETI;
}

INTERRUPT PrefetchHandler(void)
{
    RETI;
}

INTERRUPT DataHandler(void)
{
    RETI;
}

INTERRUPT UnusedHandler(void)
{
    RETI;
}

INTERRUPT FIQHandler(void)
{
    RETI;
}
