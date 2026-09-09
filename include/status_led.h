#pragma once
#include <stdint.h>
void status_led_init(void);
void status_led_set_rgb(uint8_t r,uint8_t g,uint8_t b);
void status_led_waiting(void);
void status_led_detected(void);
void status_led_dfu(void);
