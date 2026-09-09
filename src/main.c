#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "usb.h"
#include "device_probe.h"
#include "state_machine.h"
#include "status_led.h"
#include "tristar.h"
#include "pio_usb_ll.h"

static bool usb_checked = false;
static bool last_usb_connected = false;

static void probe_usb_device(void) {
    root_port_t *root = PIO_USB_ROOT_PORT(0);

    if (!root->initialized || !root->connected || usb_checked) {
        return;
    }

    /*
     * Do not interfere with the Tristar path while it is waiting
     * for the second poll. Once DFU response has been sent, USB-A
     * can be used to verify re-enumeration when a data cable is
     * physically connected.
     */
    if (!tristar_dfu_requested()) {
        return;
    }

    printf("[usb] device present on USB-A; probing DFU result\n");

    if (usb_bus_reset_open_ep0() != 0) {
        printf("[usb] EP0 reset/open failed\n");
        return;
    }

    device_probe_result_t probe = {0};
    int rc = probe_device(&probe);

    if (rc != 0) {
        printf("[usb] device probe failed: %d\n", rc);
        return;
    }

    usb_checked = true;

    printf(
        "[usb] VID=%04x PID=%04x bcdDevice=%04x\n",
        probe.vid,
        probe.pid,
        probe.bcd_device
    );

    if (probe.vid == 0x05AC && probe.pid == 0x1227) {
        recovery_led_dfu();
        printf("[usb] Apple DFU confirmed (05ac:1227)\n");
    } else {
        printf("[usb] DFU was not confirmed by USB enumeration\n");
    }
}

int main(void) {
#if PICO_RP2350
    /*
     * PIO-USB timing used by usbliter8.
     */
    set_sys_clock_khz(156000, true);
#elif PICO_RP2040
    set_sys_clock_khz(120000, true);
#else
#error Unsupported RP MCU
#endif

    stdio_init_all();

    recovery_led_init();
    recovery_sm_init();

    /*
     * Tristar runs on a dedicated PIO instance so it cannot
     * collide with the WS2812 status LED or PIO-USB.
     */
    tristar_init();

    /*
     * PIO-USB is optional during the Tristar transition.
     * It is retained for USB-side detection/verification.
     */
    sleep_ms(2000);

    usb_start();

    if (usb_bus_init() != 0) {
        printf(
            "[usb] PIO-USB initialization failed; "
            "Tristar can still operate\n"
        );
    } else {
        printf("[usb] PIO-USB host transport initialized\n");
        printf(
            "[usb] D+=GP%d D-=GP%d\n",
            PIO_USB_DP_PIN_DEFAULT,
            PIO_USB_DP_PIN_DEFAULT + 1
        );
    }

    printf(
        "[tristar] auto-DFU enabled: ID/SDQ=GP%d OE=GP%d\n",
        TRISTAR_PIN,
        TRISTAR_OE_PIN
    );

    while (true) {
        /*
         * Critical: keep servicing Tristar continuously.
         * The automatic DFU protocol is independent of USB
         * enumeration.
         */
        tristar_task();

        root_port_t *root = PIO_USB_ROOT_PORT(0);

        if (root->initialized && root->connected) {
            if (!last_usb_connected) {
                usb_checked = false;
                last_usb_connected = true;
                printf("[usb] USB-A device connected\n");
            }

            probe_usb_device();
        } else {
            if (last_usb_connected) {
                printf("[usb] USB-A device disconnected\n");
            }

            last_usb_connected = false;
            usb_checked = false;
        }

        sleep_ms(1);
    }
}
