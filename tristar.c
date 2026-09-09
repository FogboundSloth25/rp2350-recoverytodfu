#include "tristar.h"

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"

#include "tristar_rx.pio.h"
#include "tristar_tx.pio.h"

#include "status_led.h"

enum tristar_state {
    TRISTAR_COLLECT_FIRST_POLL = 0,
    TRISTAR_WAIT_SECOND_POLL,
    TRISTAR_COMPLETE
};

static PIO tristar_pio = TRISTAR_PIO_INSTANCE;
static uint tristar_sm = 0;
static enum tristar_state tristar_state_value = TRISTAR_COLLECT_FIRST_POLL;

static uint tristar_rx_offset;
static uint tristar_tx_offset;

static uint8_t request[4];
static uint request_len;

static bool transition_active;
static bool dfu_requested;

static float tristar_clkdiv(void) {
    return (float)clock_get_hz(clk_sys) / 2000000.0f;
}

static uint8_t reverse_byte(uint8_t b) {
    b = (uint8_t)(((b & 0xF0u) >> 4) | ((b & 0x0Fu) << 4));
    b = (uint8_t)(((b & 0xCCu) >> 2) | ((b & 0x33u) << 2));
    b = (uint8_t)(((b & 0xAAu) >> 1) | ((b & 0x55u) << 1));
    return b;
}

static uint8_t crc8(const uint8_t *data, size_t len) {
    uint8_t crc = 0xFF;

    while (len--) {
        crc ^= *data++;

        for (unsigned i = 0; i < 8; ++i) {
            crc = (uint8_t)(
                (crc & 0x80u) ? ((crc << 1) ^ 0x31u) : (crc << 1)
            );
        }
    }

    return crc;
}

static void configure_rx(void) {
    tristar_rx_offset = pio_add_program(
        tristar_pio,
        &tristar_rx_program
    );

    tristar_rx_program_init(
        tristar_pio,
        tristar_sm,
        tristar_rx_offset,
        TRISTAR_PIN,
        TRISTAR_OE_PIN,
        tristar_clkdiv()
    );
}

static void send_packet(const uint8_t *payload, size_t length) {
    uint8_t packet[32];

    if (length + 1 > sizeof(packet)) {
        return;
    }

    memcpy(packet, payload, length);
    packet[length] = crc8(payload, length);

    pio_sm_set_enabled(tristar_pio, tristar_sm, false);

    tristar_tx_offset = pio_add_program(
        tristar_pio,
        &tristar_tx_program
    );

    tristar_tx_program_init(
        tristar_pio,
        tristar_sm,
        tristar_tx_offset,
        TRISTAR_PIN,
        tristar_clkdiv()
    );

    pio_sm_put_blocking(
        tristar_pio,
        tristar_sm,
        packet[0]
    );

    for (size_t i = 1; i < length + 1; ++i) {
        pio_sm_put_blocking(
            tristar_pio,
            tristar_sm,
            packet[i]
        );

        (void)pio_sm_get_blocking(
            tristar_pio,
            tristar_sm
        );
    }

    (void)pio_sm_get_blocking(
        tristar_pio,
        tristar_sm
    );

    pio_sm_set_enabled(
        tristar_pio,
        tristar_sm,
        false
    );

    pio_remove_program(
        tristar_pio,
        &tristar_tx_program,
        tristar_tx_offset
    );

    pio_remove_program(
        tristar_pio,
        &tristar_rx_program,
        tristar_rx_offset
    );

    configure_rx();
}

static void send_reset_response(void) {
    static const uint8_t response[] = {
        0x75, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    printf("[tristar] RESET response\n");

    send_packet(
        response,
        sizeof(response)
    );

    transition_active = true;
    tristar_state_value = TRISTAR_WAIT_SECOND_POLL;

    recovery_led_transition();

    printf("[tristar] waiting for DFU poll\n");
}

static void send_dfu_response(void) {
    static const uint8_t response[] = {
        0x75, 0x20, 0x00, 0x02, 0x00, 0x00, 0x00
    };

    printf("[tristar] DFU response\n");

    send_packet(
        response,
        sizeof(response)
    );

    transition_active = false;
    dfu_requested = true;
    tristar_state_value = TRISTAR_COMPLETE;
    recovery_led_dfu();

    printf("[tristar] DFU request sent; waiting for Apple DFU re-enumeration\n");
}

static void send_power_response(void) {
    static const uint8_t response[] = {
        0x71, 0x93
    };

    send_packet(response, sizeof(response));

    printf("[tristar] power response\n");
}

static void send_76_response(void) {
    static const uint8_t response[] = {
        0x77, 0x02, 0x01, 0x02, 0x80, 0x60,
        0x01, 0x39, 0x3A, 0x44, 0x3E, 0xC9
    };

    send_packet(response, sizeof(response));

    printf("[tristar] 0x76 response\n");
}

static void process_request(void) {
    switch (request[0]) {
    case 0x74:
        printf(
            "[tristar] poll: %02X %02X %02X %02X\n",
            request[0], request[1], request[2], request[3]
        );

        if (tristar_state_value == TRISTAR_COLLECT_FIRST_POLL) {
            recovery_led_detected();
            send_reset_response();
        } else if (tristar_state_value == TRISTAR_WAIT_SECOND_POLL) {
            send_dfu_response();
        }
        break;

    case 0x70:
        send_power_response();
        break;

    case 0x76:
        send_76_response();
        break;

    default:
        printf(
            "[tristar] unknown request: %02X %02X %02X %02X\n",
            request[0], request[1], request[2], request[3]
        );
        break;
    }

    request_len = 0;
}

static void process_byte(uint8_t raw) {
    uint8_t value = reverse_byte(raw);

    if (request_len == 0) {
        if (value == 0x74 || value == 0x70 || value == 0x76) {
            request[0] = value;
            request_len = 1;
        }

        return;
    }

    request[request_len++] = value;

    if ((request[0] == 0x76 && request_len == 2) ||
        ((request[0] == 0x74 || request[0] == 0x70) &&
         request_len == 4)) {
        process_request();
    }
}

void tristar_init(void) {
    transition_active = false;
    dfu_requested = false;
    tristar_state_value = TRISTAR_COLLECT_FIRST_POLL;
    request_len = 0;

    gpio_init(TRISTAR_OE_PIN);
    gpio_set_dir(TRISTAR_OE_PIN, GPIO_OUT);
    gpio_put(TRISTAR_OE_PIN, 0);

    configure_rx();

    printf(
        "[tristar] listening on ID/SDQ GP%d, OE GP%d, clkdiv=%.2f\n",
        TRISTAR_PIN,
        TRISTAR_OE_PIN,
        tristar_clkdiv()
    );
}

void tristar_task(void) {
    while (!pio_sm_is_rx_fifo_empty(tristar_pio, tristar_sm)) {
        uint32_t value = pio_sm_get(
            tristar_pio,
            tristar_sm
        );

        process_byte((uint8_t)(value & 0xFFu));
    }
}

bool tristar_transition_active(void) {
    return transition_active;
}

bool tristar_dfu_requested(void) {
    return dfu_requested;
}
