/*
 * Pretty dumb mailbox system
 *   The MailboxReceive function should store the messages received in
 *   other channels instead of throwing them away. Since at this point I
 *   don't need the other channels, I keep it simple in the first place.
 */
#include <string.h>

#include "mailbox.h"
#include "pointer.h"
#include <drivers/common.h>
#include <memutils.h>
#include <utils.h>
#include <platform.h>

#define MAILBOX_ID 8
#define MAILBOX_RESPONSE_SUCCESS 0x80000000u

#define MAIL_EMPTY (1u << 30)
#define MAIL_FULL  (1u << 31)

#if RPI < 5
#define MAILBOX_BASE (PERIPHERAL_BASE + 0xb880)
#else
#define MAILBOX_BASE (PERIPHERAL_BASE + 0x13880)
#endif

#define MAIL0_READ   ((volatile uint32_t*) MAILBOX_BASE)
#define MAIL0_STATUS ((volatile uint32_t*) (MAILBOX_BASE + 0x18))
#define MAIL0_WRITE  ((volatile uint32_t*) (MAILBOX_BASE + 0x20))

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

static void mailbox_send(uint8_t channel, uint32_t data)
{
    uint32_t w = (channel & 0xF) | (data & 0xFFFFFFF0);
    wait_for_mailbox_non_full();
    memoryBarrier();
    *MAIL0_WRITE = w;
    memoryBarrier();
}

static uint32_t mailbox_receive(uint8_t channel)
{
    uint32_t data;

    do
    {
        wait_for_mailbox_non_empty();

        // Here is the dumb part
    } while(read_mail(&data) != channel);
    
    return data;
}

bool mailbox_request(const uint32_t *request, size_t size, mailbox_callback_t callback, void *context)
{
    static volatile uint32_t __attribute__((aligned(16)))
        __attribute__((section(".device"))) sequence[64];

    if(size > sizeof(sequence))
        return false;

    unaligned_vmemcpy(sequence, request, size);
    sequence[0] = size;

    // Send the requested values
    mailbox_send(MAILBOX_ID, VIDEOBUS_OFFSET + vptr_to_u32(sequence));
    if(mailbox_receive(MAILBOX_ID) == 0 || sequence[1] == MAILBOX_RESPONSE_SUCCESS)
    {
        volatile const uint32_t *ptr = sequence + 2;
        while(*ptr)
        {
            if(!callback((const void*)(ptr++), context))
                return false;
            ptr += *ptr / 4 + 2;
        }
        return true;
    }
    return false;
}

bool mailbox_ack(const uint32_t *message, void *context)
{
    (void) message;
    (void) context;
    return true;
}

