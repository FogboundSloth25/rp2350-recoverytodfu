#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "usb.h"
#include "device_probe.h"
#include "state_machine.h"
#include "status_led.h"
#include "pio_usb.h"
#include "pio_usb_ll.h"

int main(void) {
#if PICO_RP2350
    set_sys_clock_khz(156000, true);
#elif PICO_RP2040
    set_sys_clock_khz(120000, true);
#else
#error Unsupported RP MCU
#endif

    stdio_init_all();
    recovery_led_init();
    recovery_sm_init();

    sleep_ms(2000);

    usb_start();

    if (usb_bus_init() != 0) {
        printf("[usb] failed to initialize PIO-USB core1 transport\n");
        while (true) {
            sleep_ms(100);
        }
    }

    printf("[usb] PIO-USB host transport initialized\n");
    printf("[usb] D+=GP%d D-=GP%d\n",
           PIO_USB_DP_PIN_DEFAULT,
           PIO_USB_DP_PIN_DEFAULT + 1);

    while (true) {
        int rc = usb_bus_wait_for_device();
        if (rc != 0) {
            printf("[usb] wait-for-device failed: %d\n", rc);
            sleep_ms(100);
            continue;
        }

        recovery_led_detected();
        printf("[usb] device detected\n");

        if (usb_bus_reset_open_ep0() != 0) {
            printf("[usb] EP0 reset/open failed\n");
            recovery_led_waiting();
            sleep_ms(250);
            continue;
        }

        device_probe_result_t probe = {0};
        rc = probe_device(&probe);

        if (rc == 0) {
            if (probe.vid == 0x05AC && probe.pid == 0x1227) {
                recovery_led_dfu();
                printf("[usb] Apple DFU detected\n");
            } else if (probe.vid == 0x05AC &&
                       probe.pid >= 0x1280 &&
                       probe.pid <= 0x1283) {
                recovery_led_detected();
                recovery_sm_apple_device(probe.vid, probe.pid, 0);
                printf("[usb] Apple Recovery detected\n");
            } else {
                printf("[usb] non-Apple device: VID=%04x PID=%04x\n",
                       probe.vid, probe.pid);
                recovery_led_waiting();
            }
        } else {
            recovery_led_waiting();
        }

        while (true) {
            bus_timed_wait_disconnect:
            if (!PIO_USB_ROOT_PORT(0)->connected) {
                recovery_sm_device_removed();
                break;
            }
            sleep_ms(25);
        }
    }
}
