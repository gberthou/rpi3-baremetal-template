#include <app-common/splashscreen.h>
#include <drivers/bcm2835/uart.h>

void main0(void)
{
    /* This code is going to be run on core 0 */
    app_init_gpio();
    uart_init_1415();

    uart_print("Hello world!\r\n");

    app_screen_demo();
}

void main1(void)
{
    /* This code is going to be run on core 1 */
    for(;;)
    {
    }
}

void main2(void)
{
    /* This code is going to be run on core 2 */

    for(;;)
    {
    }
}

void main3(void)
{
    /* This code is going to be run on core 3 */
    for(;;)
    {
    }
}
