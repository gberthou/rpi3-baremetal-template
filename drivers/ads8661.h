#ifndef DRIVERS_ADS8661_H
#define DRIVERS_ADS8661_H

#include <stdint.h>

uint32_t ads8661_init(void);
void ads8661_write(uint16_t address, uint16_t value);
uint16_t ads8661_read(uint16_t address);

// [!] For speed purposes, return value has wrong endian 
uint32_t ads8661_sense(void);

// buf must contain at least 9 uint32_t elements.
// sizeof(buf) >= 36
void ads8661_dump(uint32_t *buf);

#endif

