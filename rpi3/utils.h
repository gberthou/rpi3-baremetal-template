#ifndef RPI3_UTILS_H
#define RPI3_UTILS_H

#define memoryBarrier() __asm__ volatile("dsb")
#define isb() __asm__ volatile("isb")

#endif

