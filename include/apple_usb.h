#pragma once
#include <stdint.h>
#include <stdbool.h>
bool apple_usb_is_recovery(uint16_t vid, uint16_t pid);
bool apple_usb_is_dfu(uint16_t vid, uint16_t pid);
bool apple_usb_is_target_recovery(uint16_t vid, uint16_t pid);
