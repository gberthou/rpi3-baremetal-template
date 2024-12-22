#include <drivers/common.h>
#include <platform.h>
#include "systimer.h"

#define SYSTEM_TIMER_BASE (PERIPHERAL_BASE + 0x00003000)

#define CLO ((volatile uint32_t*) (SYSTEM_TIMER_BASE + 0x4));
#define CHI ((volatile uint32_t*) (SYSTEM_TIMER_BASE + 0x8));

uint64_t systimer_getticks(void)
{
    uint64_t lo = *CLO;
    uint64_t hi = *CHI;
    return (hi << 32) | lo;
}

void systimer_wait_us(uint32_t us)
{
   uint64_t tend = systimer_getticks() + us;
   while(systimer_getticks() < tend);
}


