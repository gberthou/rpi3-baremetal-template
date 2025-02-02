/* Helper for exception handlers.
 * Very simple UART driver for error display purposes. The only supported
 * baudrate is 921600 baud (depending on the CPU clock). It only requires a
 * (small) stack to be setup, so this code can be reused for other purposes.
 * For instance, to print CurrentEL on boot or other kind of useful runtime or
 * architectural information.
 * This can be run at EL2 and EL1. EL3 would require an additional stub.
 * It would be strongly NOT recommended to use it in a real application since
 * it hardcodes both GPIO and UART configurations. Nevertheless, it is a
 * practical helper for displaying exception information.
 */

#include <stdint.h>

#include <platform.h>
#include <utils.h>

#define put_hex32(x) put_hex(x, 28)
#define put_hex64(x) put_hex(x, 60)

#if RPI < 5
#define GPIO_BASE (PERIPHERAL_BASE + 0x00200000)
#define UART_BASE (PERIPHERAL_BASE + 0x00201000)
#else
#define GPIO_BASE (RP1_BASE + 0xd0000)
#define PADS_BASE (RP1_BASE + 0xf0000)
#define UART_BASE (RP1_BASE + 0x30000)
#endif

static inline void putc(char c)
{
    volatile uint32_t * const UART_DR = (volatile uint32_t*) (UART_BASE);
    volatile uint32_t * const UART_FR = (volatile uint32_t*) (UART_BASE + 0x18);

    do
    {
        memoryBarrier();
    } while(*UART_FR & 0x20);
    *UART_DR = c;
    memoryBarrier();
}

static inline void putc_hex(unsigned char c)
{
    c &= 0xf;
    putc(c + (c < 10 ? '0' : 'a' - 10));
}

static inline void puts_nonewline(char *s)
{
    char c;
    while((c = *s++))
        putc(c);
}

static void put_hex(uint64_t x, uint64_t shift)
{
    do
    {
        putc_hex(x >> shift);
        shift -= 4;
    } while(shift);
    putc_hex(x);
}

void huart_init()
{
    // GPIO resistor management is different on BCM2835 and BCM2711, so let's
    // keep it simple here for portability purposes.
#if RPI < 5
    volatile uint32_t * const GPFSEL1 = (volatile uint32_t*) (GPIO_BASE + 0x4);
    *GPFSEL1 = 0x24000; // TX.14 and RX.15 in ALT0 mode
#else
    volatile uint32_t * const GPIO14_CTRL = (volatile uint32_t*) (GPIO_BASE + 0x74);
    volatile uint32_t * const GPIO15_CTRL = (volatile uint32_t*) (GPIO_BASE + 0x7c);
    volatile uint32_t * const PADS_BANK0 = (volatile uint32_t*) (PADS_BASE + 0x4);
    *GPIO14_CTRL = 4; // TX.14 in a4 mode
    *GPIO15_CTRL = 4; // RX.15 in a4 mode
    PADS_BANK0[14] = (1 << 4) | (1 << 1); // Clear output disable
    PADS_BANK0[15] = (1 << 6) | (1 << 4) | (1 << 1); // Set input enable
#endif

    volatile uint32_t * const UART_IBRD = (volatile uint32_t*) (UART_BASE + 0x24);
    volatile uint32_t * const UART_FBRD = (volatile uint32_t*) (UART_BASE + 0x28);
    volatile uint32_t * const UART_LCRH = (volatile uint32_t*) (UART_BASE + 0x2c);
    volatile uint32_t * const UART_CR = (volatile uint32_t*) (UART_BASE + 0x30);
    volatile uint32_t * const UART_ICR = (volatile uint32_t*) (UART_BASE + 0x44);

    *UART_CR = 0x0;
    *UART_ICR = 0x7ff;

    // Divider = 3.2552083
    *UART_IBRD = 0x3;
    *UART_FBRD = 0x10;

    *UART_LCRH = 0x70;
    *UART_CR = 0x300;
    *UART_CR |= 1;
}

#if __ARM_64BIT_STATE
void huart_print_exception(
    uint64_t el,
    uint64_t vbar_index,
    uint64_t esr,
    uint64_t far
)
{
    puts_nonewline("@EL: ");
    putc(el + '0');
    puts_nonewline("\r\n");

    puts_nonewline("@VBAR_ELx[");
    put_hex32(vbar_index);
    puts_nonewline("]\r\n");

    puts_nonewline("ESR_ELx = ");
    put_hex64(esr);
    puts_nonewline("\r\n");

    puts_nonewline("FAR_ELx = ");
    put_hex64(far);
    puts_nonewline("\r\n");
}
#else
void huart_print_exception(
    uint32_t mode,
    uint32_t vbar_index,
    uint32_t esr,
    uint32_t far
)
{
    puts_nonewline("@Mode: ");
    putc_hex(mode);
    puts_nonewline("\r\n");

    puts_nonewline("@VBAR_ELx[");
    put_hex32(vbar_index);
    puts_nonewline("]\r\n");

    puts_nonewline("ESR_ELx = ");
    put_hex32(esr);
    puts_nonewline("\r\n");

    puts_nonewline("FAR_ELx = ");
    put_hex32(far);
    puts_nonewline("\r\n");
}
#endif
