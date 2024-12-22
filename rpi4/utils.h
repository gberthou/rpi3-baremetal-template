#ifndef RPI4_UTILS_H
#define RPI4_UTILS_H

#define memoryBarrier() __asm__ volatile("dsb sy")
#define isb() __asm__ volatile("isb")

#endif

