#ifndef DRIVERS_MAILBOX_H
#define DRIVERS_MAILBOX_H

#include <stdint.h>

#define MAILBOX_ID 8

#define MAILBOX_RESPONSE_SUCCESS 0x80000000u

void mailbox_send(uint8_t channel, uint32_t data);
uint32_t mailbox_receive(uint8_t channel);

#endif

