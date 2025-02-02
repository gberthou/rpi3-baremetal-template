#ifndef DRIVERS_RP1_H
#define DRIVERS_RP1_H

#include <stdint.h>

#define RP1_ACCESS_OFFSET(x, offset) ((volatile uint32_t*)(((uint64_t)(x)) + (offset)))
#define RP1_ATOMIC_XOR(x) RP1_ACCESS_OFFSET(x, 0x1000)
#define RP1_ATOMIC_SET(x) RP1_ACCESS_OFFSET(x, 0x2000)
#define RP1_ATOMIC_CLR(x) RP1_ACCESS_OFFSET(x, 0x3000)

#endif // DRIVERS_RP1_H
