#include "../drivers/systimer.h"

volatile uint64_t latest_clock;

void clock_thread()
{
    for(;;)
        latest_clock = systimer_getticks();
}

