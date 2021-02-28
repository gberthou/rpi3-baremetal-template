#ifndef DRIVERS_COMMON_H
#define DRIVERS_COMMON_H

#define PERIPHERAL_BASE 0x3f000000
#define PERIPH_TO_PHYS(x) (((x) & 0x00ffffff) | 0x7e000000)
#define VIRT_TO_PHYS(x) ((x) | 0xc0000000)

#endif

