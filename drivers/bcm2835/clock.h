#ifndef DRIVERS_CLOCK_H
#define DRIVERS_CLOCK_H

#include <stdint.h>
#include <stdbool.h>

#define FIRST_CLOCK 0x1
#define LAST_CLOCK  0xe
#define N_CLOCKS (LAST_CLOCK - FIRST_CLOCK + 1)

int clock_max_out_arm(void);

#endif

