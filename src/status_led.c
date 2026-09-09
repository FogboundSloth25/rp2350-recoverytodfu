#include "status_led.h"
#include <stdbool.h>
#include "pico/status_led.h"

static bool ready;

void recovery_led_init(void) { ready = status_led_init(); }

void recovery_led_set_rgb(uint8_t r, uint8_t g, uint8_t b) {
    if (!ready || !colored_status_led_supported()) return;
    colored_status_led_set_on_with_color(
        PICO_COLORED_STATUS_LED_COLOR_FROM_RGB(r, g, b));
}

void recovery_led_waiting(void)  { recovery_led_set_rgb(255, 72, 0); }
void recovery_led_detected(void) { recovery_led_set_rgb(0, 210, 255); }
void recovery_led_dfu(void)      { recovery_led_set_rgb(0, 255, 40); }
