#include "status_led.h"
#include <stdbool.h>
#include "pico/status_led.h"

/*
 * All channel values are intentionally limited to ~50% of full scale.
 * Pico SDK converts the logical RGB value to the WS2812 wire order.
 */
#define LED_50(v) ((uint8_t)(((uint16_t)(v) * 127u) / 255u))

static bool initialized = false;

void recovery_led_init(void) {
    initialized = status_led_init();
    if (!initialized) {
        return;
    }

    colored_status_led_set_state(false);
}

void recovery_led_set_rgb(uint8_t r, uint8_t g, uint8_t b) {
    if (!initialized || !colored_status_led_supported()) {
        return;
    }

    /*
     * Use a fresh color and explicitly turn the pixel off first.
     * This avoids relying on colored_status_led_on state when changing
     * between orange/cyan/green.
     */
    colored_status_led_set_state(false);

    /* Waveshare swaps the physical red/green channels. This mirrors
     * usbliter8's LED_RED_GREEN_SWAPPED board fix. */
    colored_status_led_set_on_with_color(
        PICO_COLORED_STATUS_LED_COLOR_FROM_RGB(
            LED_50(g),
            LED_50(r),
            LED_50(b)
        )
    );
}

void recovery_led_waiting(void) {
    recovery_led_set_rgb(255, 72, 0);
}

void recovery_led_detected(void) {
    recovery_led_set_rgb(0, 210, 255);
}

void recovery_led_dfu(void) {
    recovery_led_set_rgb(0, 255, 40);
}

void recovery_led_transition(void) {
    recovery_led_set_rgb(255, 255, 0);
}
