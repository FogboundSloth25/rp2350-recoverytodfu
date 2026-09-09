#include <stdint.h>
#include "pico/multicore.h"
#include "usb.h"
#include "bus.h"

enum {
    USB_CMD_BUS_INIT = 0,
    USB_CMD_WAIT_FOR_DEVICE,
    USB_CMD_RESET_OPEN_EP0,
    USB_CMD_EXECUTE_FUNC
};

static struct {
    usb_executee_t func;
    void *ctx;
} usb_exec_ctx;

static bus_t g_bus;

static void usb_task(void) {
    while (true) {
        uint32_t cmd = multicore_fifo_pop_blocking();
        uint32_t ret = (uint32_t)-1;

        switch (cmd) {
        case USB_CMD_BUS_INIT:
            bus_init(&g_bus, false);
            ret = 0;
            break;

        case USB_CMD_WAIT_FOR_DEVICE:
            ret = bus_wait_for_connect(&g_bus) ? 0 : (uint32_t)-1;
            break;

        case USB_CMD_RESET_OPEN_EP0:
            bus_reset_ep0_reopen(&g_bus);
            ret = 0;
            break;

        case USB_CMD_EXECUTE_FUNC:
            ret = (uint32_t)usb_exec_ctx.func(&g_bus, usb_exec_ctx.ctx);
            break;

        default:
            break;
        }

        multicore_fifo_push_blocking(ret);
    }
}

void usb_start(void) {
    multicore_reset_core1();
    multicore_launch_core1(usb_task);
}

static int usb_cmd(uint32_t cmd, uint64_t timeout_us) {
    multicore_fifo_push_blocking(cmd);

    uint32_t out = (uint32_t)-1;

    if (timeout_us) {
        if (!multicore_fifo_pop_timeout_us(timeout_us, &out)) {
            return -2;
        }
    } else {
        out = multicore_fifo_pop_blocking();
    }

    return (int32_t)out;
}

int usb_bus_init(void) {
    return usb_cmd(USB_CMD_BUS_INIT, 500000);
}

int usb_bus_wait_for_device(void) {
    return usb_cmd(USB_CMD_WAIT_FOR_DEVICE, 0);
}

int usb_bus_reset_open_ep0(void) {
    return usb_cmd(USB_CMD_RESET_OPEN_EP0, 500000);
}

int usb_bus_execute(usb_executee_t func, void *ctx, uint64_t timeout) {
    usb_exec_ctx.func = func;
    usb_exec_ctx.ctx = ctx;
    return usb_cmd(USB_CMD_EXECUTE_FUNC, timeout);
}
