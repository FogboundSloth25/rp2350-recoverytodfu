#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "usb.h"
#include "device_probe.h"
#include "state_machine.h"
#include "status_led.h"
#include "tristar.h"
#include "pio_usb_ll.h"

static void check_usb_result(bool *processed) {
    root_port_t *root = PIO_USB_ROOT_PORT(0);

    if (!root->initialized || !root->connected || *processed) {
        return;
    }

    printf("[usb] device present on USB-A, probing after bus reset
");

    if (usb_bus_reset_open_ep0() != 0) {
        printf("[usb] EP0 reset/open failed
");
        return;
    }

    device_probe_result_t probe = {0};
    int rc = probe_device(&probe);

    if (rc != 0) {
        printf("[usb] device probe failed: %d
", rc);
        return;
    }

    *processed = true;

    if (probe.vid == 0x05AC &&
        probe.pid >= 0x1280 &&
        probe.pid <= 0x1283) {

        recovery_led_detected();
        recovery_sm_apple_device(probe.vid, probe.pid, 0);

        printf(
            "[usb] Apple Recovery detected: VID=%04x PID=%04x
",
            probe.vid,
            probe.pid
        );

    } else if (probe.vid == 0x05AC &&
               probe.pid == 0x1227) {

        recovery_led_dfu();

        printf(
            "[usb] Apple DFU detected: VID=%04x PID=%04x
",
            probe.vid,
            probe.pid
        );

    } else {

        printf(
            "[usb] non-Apple device: VID=%04x PID=%04x
",
            probe.vid,
            probe.pid
        );

        recovery_led_waiting();
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
     * Tristar uses PIO1 on the special Lightning ID/SDQ line.
     * It runs independently from the PIO-USB host on PIO0.
     */
    tristar_init();

    /*
     * PIO-USB host is kept for USB-A detection and final DFU
     * re-enumeration. Tristar itself does not require USB data.
     */
    sleep_ms(2000);

    usb_start();

    if (usb_bus_init() != 0) {
        printf("[usb] failed to initialize PIO-USB core1 transport
");
    } else {
        printf("[usb] PIO-USB host transport initialized
");
    }

    printf(
        "[usb] D+=GP%d D-=GP%d
",
        PIO_USB_DP_PIN_DEFAULT,
        PIO_USB_DP_PIN_DEFAULT + 1
    );

    printf(
        "[tristar] auto-DFU enabled on GP%d
",
        TRISTAR_PIN
    );

    bool usb_processed = false;
    bool last_usb_connected = false;

    while (true) {
        /*
         * This must run continuously: the Tristar request can
         * arrive at any time while the phone is in Recovery.
         */
        tristar_task();

        root_port_t *root = PIO_USB_ROOT_PORT(0);

        if (root->initialized && root->connected) {
            if (!last_usb_connected) {
                usb_processed = false;
                last_usb_connected = true;
                printf("[usb] USB-A device connected
");
            }

            /*
             * Only probe when a Tristar transition has completed
             * or when a device is independently connected in DFU.
             */
            if (tristar_dfu_requested() || !tristar_transition_active()) {
                check_usb_result(&usb_processed);
            }

        } else {
            if (last_usb_connected) {
                printf("[usb] USB-A device disconnected
");
            }

            last_usb_connected = false;
            usb_processed = false;
        }

        sleep_ms(1);
    }
}
