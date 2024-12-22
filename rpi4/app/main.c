#include <string.h>

#include <app-common/splashscreen.h>
#include <drivers/mem.h>
#include <drivers/bcm2835/uart.h>
#include <drivers/bcm2835/systimer.h>

#define WAIT_TIME_US 100000

static __attribute__((section(".exclusive"))) volatile uint32_t core_data = 0;
static void atomic_or(volatile uint32_t *dst, uint32_t src)
{
    // Can use LDSET instead, starting from Armv8.1

    uint32_t status = 1;
    do
    {
        __asm__ ("ldaxr w2, [%x1]\n"
                 "orr w2, w2, %w2\n"
                 "stlxr %w0, w2, [%x1]"
                 : "=&r"(status)
                 : "r"(dst), "r"(src)
                 : "w2"
        );
        if (status == 0)
            __asm__("sev");
        else
            __asm__("wfe");
    }
    while(status != 0);
}

void main0(void)
{
    /* This code is going to be run on core 0 */
    mem_init();
    app_init_gpio();
    uart_init_1415();

    uart_print("Hello world!\r\n");
    systimer_wait_us(WAIT_TIME_US);
    atomic_or(&core_data, 1);
    {
        char s[16] = {0};
        char c = core_data & 0xf;
        strcpy(s, "Core check: ?\r\n");
        s[12] = (c < 10 ? '0' : 'a' - 10) + c;
        uart_print(s); // Should print "f" if all cores responded in time
    }

    app_screen_demo();
}

void main1(void)
{
    /* This code is going to be run on core 1 */
    atomic_or(&core_data, 2);

    for(;;)
    {
        __asm__("wfe");
    }
}

void main2(void)
{
    /* This code is going to be run on core 2 */
    atomic_or(&core_data, 4);

    for(;;)
    {
        __asm__("wfe");
    }
}

void main3(void)
{
    /* This code is going to be run on core 3 */
    atomic_or(&core_data, 8);

    for(;;)
    {
        __asm__("wfe");
    }
}
