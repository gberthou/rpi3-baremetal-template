#ifndef RPI1_UTILS_H
#define RPI1_UTILS_H

#define memoryBarrier() __asm__ volatile("mcr p15, 0, %0, c7, c10, 5" :: "r"(0))
#define isb() __asm__ volatile("mcr p15, 0, r0, c7, c5, 4")

#endif

