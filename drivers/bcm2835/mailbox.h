#ifndef DRIVERS_MAILBOX_H
#define DRIVERS_MAILBOX_H

#include <stdint.h>

void mailbox_send(uint8_t channel, uint32_t data);
uint32_t mailbox_receive(uint8_t channel);

#endif

