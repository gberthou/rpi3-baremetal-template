#include <stdint.h>

uint32_t is_qemu;

void init_is_qemu()
{
#if RPI == 1
    is_qemu = 0;
#else
    extern uint8_t __btext;
    uint32_t text_address = (uint32_t) &__btext;
    is_qemu = (text_address == 0x10000 ? 1 : 0);
#endif
}

