#ifndef SYSTEM_TIMER_H
#define SYSTEM_TIMER_H

#include <stdint.h>

uint64_t systimer_getticks(void);
void systimer_wait_us(uint32_t us);

#endif

