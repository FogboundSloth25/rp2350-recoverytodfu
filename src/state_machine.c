#include "state_machine.h"
#include "apple_usb.h"
#include "status_led.h"
#include <stdio.h>
static recovery_state_t state = RP2350_REC_WAIT_FOR_DEVICE;
static uint8_t device_address = 0;
void recovery_sm_init(void) {
    state = RP2350_REC_WAIT_FOR_DEVICE;
    device_address = 0;
    recovery_led_waiting();
    printf("[sm] WAIT_FOR_DEVICE\n");
}
recovery_state_t recovery_sm_state(void) { return state; }
void recovery_sm_apple_device(uint16_t vid, uint16_t pid, uint8_t address) {
    device_address = address;
    if (apple_usb_is_dfu(vid, pid)) {
        state = RP2350_REC_DFU_READY;
        recovery_led_dfu();
        printf("[sm] DFU_READY addr=%u vid=%04x pid=%04x\n", address, vid, pid);
        return;
    }
    if (apple_usb_is_target_recovery(vid, pid)) {
        state = RP2350_REC_APPLE_RECOVERY_DETECTED;
        recovery_led_detected();
        printf("[sm] APPLE_RECOVERY_DETECTED addr=%u vid=%04x pid=%04x\n", address, vid, pid);
        state = RP2350_REC_IDENTIFY_TARGET;
        printf("[sm] IDENTIFY_TARGET\n");
        state = RP2350_REC_TRANSITION_PENDING;
        printf("[sm] TRANSITION_PENDING\n");
        state = RP2350_REC_WAIT_FOR_DFU;
        printf("[sm] WAIT_FOR_DFU\n");
    }
}
void recovery_sm_device_removed(void) {
    device_address = 0;
    state = RP2350_REC_WAIT_FOR_DEVICE;
    recovery_led_waiting();
    printf("[sm] device removed -> WAIT_FOR_DEVICE\n");
}
void recovery_sm_tick(void) { (void)device_address; }
