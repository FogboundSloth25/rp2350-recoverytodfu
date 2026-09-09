#include <stdio.h>
#include <pico/stdlib.h>
#include <tusb.h>
#include "state_machine.h"
#include "status_led.h"
void tuh_mount_cb(uint8_t dev_addr) {
    uint16_t vid = 0, pid = 0;
    if (tuh_vid_pid_get(dev_addr, &vid, &pid)) {
        printf("[usb] mount addr=%u vid=%04x pid=%04x\n", dev_addr, vid, pid);
        recovery_sm_apple_device(vid, pid, dev_addr);
    } else {
        printf("[usb] mount addr=%u (descriptor unavailable yet)\n", dev_addr);
    }
}
void tuh_umount_cb(uint8_t dev_addr) {
    printf("[usb] unmount addr=%u\n", dev_addr);
    recovery_sm_device_removed();
}
int main(void) {
    stdio_init_all();
    sleep_ms(250);
    printf("RP2350 Recovery\n");
    printf("Target: iPhone 11 Pro / A13\n");
    status_led_init();
    recovery_sm_init();
    tuh_init(0);
    while (true) {
        tuh_task();
        recovery_sm_tick();
        sleep_ms(1);
    }
}
