#ifndef BACKLIGHT_H
#define BACKLIGHT_H

#include <stdint.h>

void backlight_init(void);
void backlight_set_brightness(uint8_t brightness);
uint8_t backlight_get_brightness(void);

#endif // BACKLIGHT_H
