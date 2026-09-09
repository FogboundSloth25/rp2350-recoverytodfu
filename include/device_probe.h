#pragma once
#include <stdint.h>
typedef struct {
    uint16_t vid;
    uint16_t pid;
    uint16_t bcd_device;
    uint8_t max_packet_size;
} device_probe_result_t;
int probe_device(device_probe_result_t *result);
