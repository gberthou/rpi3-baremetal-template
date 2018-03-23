#include "common.h"
#include "systimer.h"

#define SYSTEM_TIMER_BASE (PERIPHERAL_BASE + 0x00003000)

volatile uint64_t * const TICKS = (uint64_t*) (SYSTEM_TIMER_BASE + 0x4);

uint64_t systimer_getticks(void)
{
    return *TICKS;
}

