#ifndef RPI1_UTILS_H
#define RPI1_UTILS_H

#define memoryBarrier() __asm__ volatile("mcr p15, 0, %0, c7, c10, 5" :: "r"(0))

#endif

