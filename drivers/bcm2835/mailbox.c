/*
 * Pretty dumb mailbox system
 *   The MailboxReceive function should store the messages received in
 *   other channels instead of throwing them away. Since at this point I
 *   don't need the other channels, I keep it simple in the first place.
 */

#include "mailbox.h"
#include <drivers/common.h>

#if 0
#define memoryBarrier() __asm__ volatile("mcr p15, 0, %0, c7, c10, 5" :: "r"(0))
#else
#define memoryBarrier() __asm__ volatile("dsb")
#endif

#define MAIL_EMPTY (1u << 30)
#define MAIL_FULL  (1u << 31)

#define MAIL0_READ   ((volatile uint32_t*) (PERIPHERAL_BASE + 0x0000b880))
#define MAIL0_STATUS ((volatile uint32_t*) (PERIPHERAL_BASE + 0x0000b898))
#define MAIL0_WRITE  ((volatile uint32_t*) (PERIPHERAL_BASE + 0x0000b8A0))

static inline void wait_for_mailbox_non_full(void)
{
    uint32_t status;
    do
    {
        memoryBarrier();
        status = *MAIL0_STATUS;
        memoryBarrier();
    } while(status & MAIL_FULL);
}

static inline void wait_for_mailbox_non_empty(void)
{
    uint32_t status;
    do
    {
        memoryBarrier();
        status = *MAIL0_STATUS;
        memoryBarrier();
    } while(status & MAIL_EMPTY);
}

static inline uint8_t read_mail(uint32_t *data)
{
    uint32_t mail;
    memoryBarrier();
    mail = *MAIL0_READ;
    memoryBarrier();
    
    *data = (mail & 0xFFFFFFF0);
    return mail & 0xF;
}

void mailbox_send(uint8_t channel, uint32_t data)
{
    uint32_t w = (channel & 0xF) | (data & 0xFFFFFFF0);
    wait_for_mailbox_non_full();
    memoryBarrier();
    *MAIL0_WRITE = w;
    memoryBarrier();
}

uint32_t mailbox_receive(uint8_t channel)
{
    uint32_t data;

    do
    {
        wait_for_mailbox_non_empty();

        // Here is the dumb part
    } while(read_mail(&data) != channel);
    
    return data;
}
