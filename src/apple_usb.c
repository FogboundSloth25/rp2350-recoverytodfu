#include "apple_usb.h"
#define USB_VID_APPLE 0x05ACu
bool apple_usb_is_recovery(uint16_t vid, uint16_t pid) {
    return vid == USB_VID_APPLE && pid >= 0x1280u && pid <= 0x1283u;
}
bool apple_usb_is_dfu(uint16_t vid, uint16_t pid) {
    return vid == USB_VID_APPLE && pid == 0x1227u;
}
bool apple_usb_is_target_recovery(uint16_t vid, uint16_t pid) {
    return apple_usb_is_recovery(vid, pid);
}
