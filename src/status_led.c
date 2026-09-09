#include "status_led.h"
#include "pico/status_led.h"

static bool ready;

void status_led_init(void) {
    ready = status_led_init();
}

void status_led_set_rgb(uint8_t r, uint8_t g, uint8_t b) {
    if (!ready) return;
    // SDK API uses 0xWWRRGGBB. Waveshare's board definition handles
    // the WS2812 pin; channel ordering follows the board's upstream fix.
    (void)colored_status_led_set_on_with_color(
        PICO_COLORED_STATUS_LED_COLOR_FROM_RGB(r, g, b));
}

void status_led_waiting(void)  { status_led_set_rgb(255, 72, 0); }
void status_led_detected(void) { status_led_set_rgb(0, 210, 255); }
void status_led_dfu(void)      { status_led_set_rgb(0, 255, 40); }
