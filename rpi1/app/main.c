#include <app-common/splashscreen.h>
#include <drivers/mem.h>
#include <drivers/bcm2835/uart.h>

void main(void)
{
    mem_init();
    app_init_gpio();
    uart_init_1415();

    uart_print("Hello world!\r\n");

    app_screen_demo();
}

