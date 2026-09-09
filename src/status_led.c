#include "status_led.h"
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"

#ifndef LED_PIN
#define LED_PIN 16
#endif

#ifndef LED_RGB_ORDER_GRB
#define LED_RGB_ORDER_GRB 1
#endif

#if LED_PIN >= 0
static PIO led_pio = pio2;
static uint led_sm = 0;
static uint led_offset = 0;
static bool led_ready = false;
#endif

static uint32_t pack_pixel(uint8_t r, uint8_t g, uint8_t b) {
#if LED_RGB_ORDER_GRB
    return ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;
#else
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
#endif
}

void status_led_init(void) {
#if LED_PIN >= 0
    led_offset = pio_add_program(led_pio, &ws2812_program);
    pio_sm_config c = ws2812_program_get_default_config(led_offset);
    sm_config_set_sideset_pins(&c, LED_PIN);
    sm_config_set_out_shift(&c, false, true, 24);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    sm_config_set_clkdiv(&c, (float)clock_get_hz(clk_sys) / 800000.0f / 10.0f);
    pio_gpio_init(led_pio, LED_PIN);
    pio_sm_set_consecutive_pindirs(led_pio, led_sm, LED_PIN, 1, true);
    pio_sm_init(led_pio, led_sm, led_offset, &c);
    pio_sm_set_enabled(led_pio, led_sm, true);
    led_ready = true;
    status_led_set_rgb(0, 0, 0);
#endif
}

void status_led_set_rgb(uint8_t r, uint8_t g, uint8_t b) {
#if LED_PIN >= 0
    if (!led_ready) return;
    pio_sm_put_blocking(led_pio, led_sm, pack_pixel(r, g, b) << 8);
#else
    (void)r; (void)g; (void)b;
#endif
}

void status_led_waiting(void)  { status_led_set_rgb(255, 72, 0); }
void status_led_detected(void) { status_led_set_rgb(0, 210, 255); }
void status_led_dfu(void)      { status_led_set_rgb(0, 255, 40); }
