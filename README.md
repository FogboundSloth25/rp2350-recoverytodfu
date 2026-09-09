# rp2350-recoverytodfu

RP2350 firmware research project for Apple USB-mode detection.

## RGB status LED

Waveshare RP2350 USB-A has its onboard WS2812 data input on GPIO16.

- Orange: waiting
- Cyan: Apple Recovery detected
- Green: Apple DFU detected

The green state is only asserted after actual USB enumeration as Apple DFU. The Apple Recovery -> DFU transition logic is not implemented in this status-LED-only change.

## Build

Run `./build.sh`. The complete stdout/stderr transcript is saved as `build.log`.

Waveshare documents the board's USB-A host port and onboard WS2812; GPIO16 is the WS2812 data input. 
