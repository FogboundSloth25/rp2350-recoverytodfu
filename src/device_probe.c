#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "bus.h"
#include "usb_definitions.h"
typedef struct {
    device_probe_result_t result;
} probe_ctx_t;

int probe_device_internal(bus_t *b, void *ctx_ptr) {
    probe_ctx_t *ctx = (probe_ctx_t *)ctx_ptr;
    uint8_t descriptor[18] = {0};

    uint8_t setup[8] = {
        USB_REQ_DIR_IN,
        0x06,
        0x00,
        0x01,
        0x00,
        0x00,
        0x12,
        0x00
    };

    int rc = bus_control_xfer(b, setup, descriptor, sizeof(descriptor), true, 250);
    if (rc != 0) {
        printf("[usb] GET_DESCRIPTOR failed: %d operation=%d stage=%d\n",
               rc,
               b->dev->control_pipe.operation,
               b->dev->control_pipe.stage);
        return rc;
    }

    ctx->result.vid =
        (uint16_t)descriptor[8] |
        ((uint16_t)descriptor[9] << 8);

    ctx->result.pid =
        (uint16_t)descriptor[10] |
        ((uint16_t)descriptor[11] << 8);

    ctx->result.bcd_device =
        (uint16_t)descriptor[12] |
        ((uint16_t)descriptor[13] << 8);

    ctx->result.max_packet_size = descriptor[7];

    printf("[usb] VID=%04x PID=%04x bcdDevice=%04x maxpkt0=%u\n",
           ctx->result.vid,
           ctx->result.pid,
           ctx->result.bcd_device,
           ctx->result.max_packet_size);

    return 0;
}

int probe_device(device_probe_result_t *result) {
    probe_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    int rc = usb_bus_execute(probe_device_internal, &ctx, 1000000);
    if (rc == 0) {
        *result = ctx.result;
    }
    return rc;
}
