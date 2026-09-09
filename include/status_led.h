#pragma once
#include <stdint.h>
void recovery_led_init(void);
void recovery_led_set_rgb(uint8_t r, uint8_t g, uint8_t b);
void recovery_led_waiting(void);
void recovery_led_detected(void);
void recovery_led_dfu(void);
void recovery_led_transition(void);
