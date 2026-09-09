# rp2350-recoverytodfu

Research firmware for RP2350 boards, with the Waveshare RP2350 USB-A as the primary target.

## Waveshare RP2350 USB-A fixes

This project uses the same board-specific USB architecture as the upstream usbliter8 project:

- USB-A PIO-USB D+ = GPIO12
- USB-A PIO-USB D- = GPIO13
- onboard WS2812 = GPIO16
- RP2350 system clock = 156 MHz for PIO-USB timing
- PIO-USB runs separately from the native USB CDC debug connection

Waveshare documents the USB-A connector as PIO-USB host/device. For host mode, Waveshare's documentation says R13 must be removed; the board's host-mode implementation also uses the GPIO12/13 data pins. A hardware host modification may therefore be required before an iPhone is detected.

### Hardware host mod

For the Waveshare RP2350-USB-A host port, verify the board modification before debugging firmware:

1. Remove R13 (the D+ pull-up).
2. Add 15 kOhm pull-down from D+ / GPIO12 to GND.
3. Add 15 kOhm pull-down from D- / GPIO13 to GND.

Without the host electrical configuration, PIO-USB may never see the iPhone.

## RGB status

- orange = waiting
- cyan = Apple Recovery detected
- green = Apple DFU detected

The green state means the Apple DFU USB device was actually enumerated. It does not claim that a Recovery-to-DFU transition command has succeeded yet.

## Build

`./build.sh` opens the board selector when no board is supplied and always writes the complete stdout/stderr transcript to `build.log`.

Supported board profiles:

- Waveshare RP2350 USB-A
- Waveshare RP2350 Zero
- Pimoroni TINY2350
- Raspberry Pi Pico 2

The PIO-USB implementation is vendored from the upstream usbliter8 project so that board-specific timing/transport fixes are not lost.
