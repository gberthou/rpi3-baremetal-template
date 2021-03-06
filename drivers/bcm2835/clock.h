#ifndef DRIVERS_CLOCK_H
#define DRIVERS_CLOCK_H

#include <stdint.h>

#define FIRST_CLOCK 0x1
#define LAST_CLOCK  0xe
#define N_CLOCKS (LAST_CLOCK - FIRST_CLOCK + 1)

struct clock_data_t
{
    uint32_t state[N_CLOCKS];
    uint32_t rate[N_CLOCKS];
};

int clock_max_out_arm(void);
int clock_dump(struct clock_data_t *data);

#endif

