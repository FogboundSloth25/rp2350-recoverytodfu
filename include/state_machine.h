#pragma once
#include <stdint.h>
typedef enum {
    RP2350_REC_WAIT_FOR_DEVICE = 0,
    RP2350_REC_APPLE_RECOVERY_DETECTED,
    RP2350_REC_IDENTIFY_TARGET,
    RP2350_REC_TRANSITION_PENDING,
    RP2350_REC_WAIT_FOR_DFU,
    RP2350_REC_DFU_READY,
    RP2350_REC_ERROR
} recovery_state_t;
void recovery_sm_init(void);
void recovery_sm_tick(void);
recovery_state_t recovery_sm_state(void);
void recovery_sm_apple_device(uint16_t vid, uint16_t pid, uint8_t address);
void recovery_sm_device_removed(void);
