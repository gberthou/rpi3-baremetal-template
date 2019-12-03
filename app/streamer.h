#ifndef STREAMER_H
#define STREAMER_H

#include <stdbool.h>

void streamer_acquisition_thread(bool summary);
void streamer_gpio_thread(void);
void streamer_display_thread(void);

#endif

