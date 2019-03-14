#ifndef DRIVERS_UART_H
#define DRIVERS_UART_H

#include "common.h"

void uart_init_1415(void);
void uart_putc(uint8_t c);
void uart_print(const char *str);
void uart_print_hex(uint64_t x);

// [!] size must be multiple of 4
void uart_print_raw32(const void *data, size_t size);

#endif

