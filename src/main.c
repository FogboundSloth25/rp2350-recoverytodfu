#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "pio_usb.h"
#include "usb_definitions.h"

#include "state_machine.h"
#include "status_led.h"

#define APPLE_VID 0x05ACu
#define APPLE_RECOVERY_PID_MIN 0x1280u
#define APPLE_RECOVERY_PID_MAX 0x1283u
#define APPLE_DFU_PID 0x1227u

struct usb_device_descriptor_host {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdUSB;
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint8_t  bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t  iManufacturer;
    uint8_t  iProduct;
    uint8_t  iSerialNumber;
    uint8_t  bNumConfigurations;
} __attribute__((packed));

static bool apple_recovery_pid(uint16_t vid, uint16_t pid) {
    return vid == APPLE_VID &&
           pid >= APPLE_RECOVERY_PID_MIN &&
           pid <= APPLE_RECOVERY_PID_MAX;
}

static bool apple_dfu_pid(uint16_t vid, uint16_t pid) {
    return vid == APPLE_VID && pid == APPLE_DFU_PID;
}

static int control_in(uint8_t device_address,
                      uint8_t *buffer,
                      uint16_t length,
                      const uint8_t setup[8]) {
    usb_device_t *dev = &pio_usb_device[0];

    dev->control_pipe.stage = STAGE_SETUP;
    dev->control_pipe.operation = CONTROL_IN;
    dev->control_pipe.rx_buffer = buffer;
    dev->control_pipe.request_length = length;
    dev->control_pipe.out_data_packet.tx_address = NULL;
    dev->control_pipe.out_data_packet.tx_length = 0;

    if (!pio_usb_host_send_setup(0, device_address, setup)) {
        return -1;
    }

    absolute_time_t deadline = make_timeout_time_ms(250);

    while (dev->control_pipe.operation != CONTROL_COMPLETE &&
           dev->control_pipe.operation != CONTROL_ERROR) {
        if (time_reached(deadline)) {
            return -2;
        }
        tight_loop_contents();
    }

    return dev->control_pipe.operation == CONTROL_COMPLETE ? 0 : -1;
}

static int read_device_descriptor(usb_device_t *dev,
                                  struct usb_device_descriptor_host *desc) {
    static const uint8_t setup[8] = {
        0x80, 0x06, 0x00, 0x01, 0x00, 0x00,
        sizeof(struct usb_device_descriptor_host), 0x00
    };

    return control_in(dev->address, (uint8_t *)desc, sizeof(*desc), setup);
}

int main(void) {
#if PICO_RP2350
    // PIO-USB timing used by usbliter8 requires a 156 MHz system clock.
    set_sys_clock_khz(156000, true);
#elif PICO_RP2040
    set_sys_clock_khz(120000, true);
#else
#error Unsupported RP MCU
#endif

    stdio_init_all();
    recovery_led_init();
    recovery_led_waiting();
    recovery_sm_init();

    sleep_ms(2000);

    pio_usb_configuration_t cfg = PIO_USB_DEFAULT_CONFIG;
    cfg.skip_alarm_pool = false;

    usb_device_t *dev = pio_usb_host_init(&cfg);
    root_port_t *root = PIO_USB_ROOT_PORT(0);

    if (!dev) {
        printf("[usb] PIO-USB init failed\n");
        while (true) {
            sleep_ms(100);
        }
    }

    printf("[usb] PIO-USB host started: D+=GP%d D-=GP%d\n",
           PIO_USB_DP_PIN_DEFAULT, PIO_USB_DP_PIN_DEFAULT + 1);

    bool previous_connected = false;
    bool device_processed = false;

    while (true) {
        if (!root->initialized) {
            sleep_ms(1);
            continue;
        }

        if (!root->connected) {
            if (previous_connected) {
                printf("[usb] device disconnected\n");
                device_processed = false;
                recovery_sm_device_removed();
            }
            previous_connected = false;
            sleep_ms(5);
            continue;
        }

        previous_connected = true;

        if (!device_processed) {
            printf("[usb] device detected, resetting bus\n");

            pio_usb_host_port_reset_start(0);
            sleep_ms(20);
            pio_usb_host_port_reset_end(0);
            sleep_ms(50);

            dev->connected = true;
            dev->enumerated = false;
            dev->is_fullspeed = root->is_fullspeed;
            dev->is_root = true;
            dev->root = root;
            dev->address = 0;

            endpoint_descriptor_t ep0 = {
                .length = 7,
                .type = DESC_TYPE_ENDPOINT,
                .epaddr = 0x00,
                .attr = EP_ATTR_CONTROL,
                .max_size = {64, 0},
                .interval = 0
            };

            if (!pio_usb_host_endpoint_open(0, dev->address,
                                            (const uint8_t *)&ep0, false)) {
                printf("[usb] failed to open EP0\n");
                sleep_ms(100);
                continue;
            }

            struct usb_device_descriptor_host desc;
            memset(&desc, 0, sizeof(desc));

            int rc = read_device_descriptor(dev, &desc);
            if (rc != 0) {
                printf("[usb] GET_DESCRIPTOR failed: %d\n", rc);
                sleep_ms(100);
                continue;
            }

            dev->vid = desc.idVendor;
            dev->pid = desc.idProduct;
            dev->enumerated = true;
            device_processed = true;

            printf("[usb] VID=%04x PID=%04x bcdDevice=%04x maxpkt0=%u\n",
                   dev->vid, dev->pid, desc.bcdDevice, desc.bMaxPacketSize0);

            recovery_sm_apple_device(dev->vid, dev->pid, dev->address);

            if (apple_dfu_pid(dev->vid, dev->pid)) {
                recovery_led_dfu();
            } else if (apple_recovery_pid(dev->vid, dev->pid)) {
                recovery_led_detected();
            }
        }

        sleep_ms(1);
    }
}
