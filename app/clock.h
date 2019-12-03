#ifndef CLOCK_H
#define CLOCK_H

extern volatile uint64_t latest_clock;

void clock_thread();

#endif

