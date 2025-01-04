#ifndef PLATFORM_H
#define PLATFORM_H

#if __ARM_64BIT_STATE
#define PERIPHERAL_BASE 0xfe000000
#else
#include <stdint.h>
extern uint32_t is_qemu;
#define PERIPHERAL_BASE (is_qemu ? 0x3f000000 : 0xfe000000)
#endif

#endif
