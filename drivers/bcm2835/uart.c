#include <stdint.h>
#include <stddef.h>

#include <drivers/common.h>
#include "gpio.h"

#define UART_BASE (PERIPHERAL_BASE + 0x00201000)

#define GPIO_TX 14
#define GPIO_RX 15

#define UART_CLOCK 48000000 // Default value, modifiable using `init_uart_clock` parameter of boot/config.txt
#define BAUDRATE   31250

#define BRD_VALUE ((float)UART_CLOCK / (16.f * BAUDRATE))
#define IBRD_VALUE ((unsigned int) BRD_VALUE)
#define FBRD_VALUE ((unsigned int) ((BRD_VALUE - IBRD_VALUE) * 64.f + 0.5f))

#define DR   ((volatile uint32_t*)(UART_BASE + 0x00))
#define FR   ((volatile uint32_t*)(UART_BASE + 0x18))
#define IBRD ((volatile uint32_t*)(UART_BASE + 0x24))
#define FBRD ((volatile uint32_t*)(UART_BASE + 0x28))
#define LCRH ((volatile uint32_t*)(UART_BASE + 0x2c))
#define CR   ((volatile uint32_t*)(UART_BASE + 0x30))
#define ICR  ((volatile uint32_t*)(UART_BASE + 0x44))

/* Most of the code is inspired from https://github.com/dwelch67/raspberrypi/blob/master/uartx01/uartx01.c */

static const char FIGURES[] = "0123456789abcdef";

void uart_init_1415(void)
{
    *CR = 0x00;

    gpio_select_function(GPIO_TX, GPIO_ALT0);
    gpio_select_function(GPIO_RX, GPIO_ALT0);

    gpio_set_resistor(GPIO_TX, GPIO_RESISTOR_NONE);
    gpio_set_resistor(GPIO_RX, GPIO_RESISTOR_NONE);

    *ICR  = 0x7FF; // Clear UART interrupt flags
    *IBRD = IBRD_VALUE;
    *FBRD = FBRD_VALUE; 
    *LCRH = 0x70;  // Fifos enabled | 8bit words
    *CR   = 0x301; // RX enable | TX enable | UART enabled
}

void uart_putc(uint8_t c)
{
    while(*FR & 0x20);
    *DR = c;
}

uint8_t uart_getc(void)
{
    while(*FR & 0x02);
    return *DR;
}

void uart_print(const char *str)
{
    char c;
    while((c = *str++))
        uart_putc(c);
}

void uart_error(const char *str)
{
    uart_print(str);
    for(;;)
        __asm__ __volatile__("nop");
}

static void u64_to_hex(char *dst, uint64_t x)
{
    for(unsigned int i = 0; i < 16; ++i)
    {
        dst[15 - i] = FIGURES[x & 0xf];
        x >>= 4;
    }
}

void uart_print_hex(uint64_t x)
{
    char tmp[19];
    tmp[16] = '\r';
    tmp[17] = '\n';
    tmp[18] = 0;

    u64_to_hex(tmp, x);
    uart_print(tmp);
}

void uart_print_raw32(const void *data, size_t size)
{
    const uint32_t *_data = data;
    size >>= 2;

    while(size--)
    {
        uint32_t value = *_data++;
        for(size_t i = 8; i-- > 0;)
            uart_putc(FIGURES[(value >> (i << 2)) & 0xf]);
    }
    uart_putc('\r');
    uart_putc('\n');
}

