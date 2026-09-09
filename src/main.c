#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/clocks.h"
#include "pio_usb.h"
#include "usb_definitions.h"
#include "state_machine.h"
#include "status_led.h"

static usb_device_t *g_dev;

static void usb_core1_task(void) {
    pio_usb_host_task();
}

int main(void) {
#if PICO_RP2350
    // PIO-USB timing requires a clock that is an exact multiple of 12 MHz.
    set_sys_clock_khz(156000, true);
#elif PICO_RP2040
    set_sys_clock_khz(120000, true);
#else
#error Unsupported MCU
#endif

    stdio_init_all();
    recovery_led_init();
    recovery_led_waiting();
    recovery_sm_init();

    sleep_ms(2000);

    pio_usb_configuration_t cfg = PIO_USB_DEFAULT_CONFIG;
    cfg.skip_alarm_pool = false;
    g_dev = pio_usb_host_init(&cfg);
    if (!g_dev) {
        printf("[usb] PIO-USB init failed\n");
        while (true) { sleep_ms(100); }
    }

    printf("[usb] PIO-USB host: DP=GP%d DM=GP%d\n",
           PIO_USB_DP_PIN_DEFAULT, PIO_USB_DP_PIN_DEFAULT + 1);
    printf("[usb] board: %s\n", BOARD_NAME);

    multicore_launch_core1(usb_core1_task);

    bool was_connected = false;
    bool was_enumerated = false;
    uint16_t old_vid = 0, old_pid = 0;

    while (true) {
        bool connected = g_dev->connected;
        bool enumerated = g_dev->enumerated;

        if (!connected) {
            if (was_connected) {
                printf("[usb] device disconnected\n");
                recovery_sm_device_removed();
            }
            was_connected = false;
            was_enumerated = false;
        } else if (enumerated) {
            if (!was_enumerated || g_dev->vid != old_vid || g_dev->pid != old_pid) {
                printf("[usb] device VID=%04x PID=%04x addr=%u speed=%s\n",
                       g_dev->vid, g_dev->pid, g_dev->address,
                       g_dev->is_fullspeed ? "FS" : "LS");
                recovery_sm_apple_device(g_dev->vid, g_dev->pid, g_dev->address);
                old_vid = g_dev->vid;
                old_pid = g_dev->pid;
                was_enumerated = true;
            }
            was_connected = true;
        } else {
            was_connected = true;
        }

        tight_loop_contents();
    }
}
