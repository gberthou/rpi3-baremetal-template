#include <string.h>

#include <utils.h>
#include <app-common/splashscreen.h>
#include <drivers/mem.h>
#include <drivers/bcm2835/uart.h>
#include <drivers/bcm2835/systimer.h>

#define WAIT_TIME_US 100000

#if __ARM_64BIT_STATE
static __attribute__((section(".exclusive"))) volatile uint32_t core_data = 0;
#else
#define MUTEX_FREE 0
#define MUTEX_TAKEN 1
static volatile uint32_t core_data = 0;
static __attribute__((section(".exclusive"))) volatile uint32_t mutex = MUTEX_FREE;
#endif

static void atomic_or(volatile uint32_t *dst, uint32_t src)
{
#if __ARM_64BIT_STATE
#if RPI < 5
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
        if (status != 0)
            __asm__("wfe");
    }
    while(status != 0);
    __asm__("sev");
#else
    __asm__("ldset %x0, xzr, [%x1]"
            :: "r"(src), "r"(dst)
    );
#endif
#else
    uint32_t status = 1;
    // First, claim mutex
    do
    {
        __asm__ __volatile__(
            "ldrex %0, %1\n" // The fact that MUTEX_TAKEN=1 makes the returned
                             // value compatible with the status value
                             // returned by strex
            "cmp %0, %3\n"
            "wfene\n" // If the mutex wasn't MUTEX_FREE, wait
            "strexeq %0, %2, %1" // Else, try to claim it
            : "=r"(status)
            : "m"(mutex), "r"(MUTEX_TAKEN), "i"(MUTEX_FREE)
        );
    }
    while(status != 0);
    memoryBarrier();

    // Then, change value
    *dst |= src;

    // Finally, release mutex
    memoryBarrier();
    mutex = MUTEX_FREE;
    __asm__("sev");
#endif
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

#if RPI > 1
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
#endif
