#ifndef DRIVERS_MAILBOX_H
#define DRIVERS_MAILBOX_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Please see
 * https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
 */

typedef bool (*mailbox_callback_t)(const uint32_t *, void *);

bool mailbox_request(const uint32_t *request, size_t size, mailbox_callback_t callback, void *context);

bool mailbox_ack(const uint32_t *message, void *context);

#endif

