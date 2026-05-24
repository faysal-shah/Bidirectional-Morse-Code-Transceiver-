# Firmware

The Arduino sketch lives in `morse_transceiver/`.
The folder name **must** match the `.ino` file name — this is an Arduino IDE requirement.

## File Overview

| File | Purpose |
|------|---------|
| `morse_transceiver.ino` | Entry point — `setup()` and `loop()`; wires all modules together |
| `config.h` | All compile-time settings: pins, timing, encryption keys, device ID |
| `morse_code.h` | ITU Morse encode/decode lookup tables and helper functions |
| `crypto.h` | AES-128-CBC encryption + HMAC-SHA256 authentication via mbedTLS |
| `comm.h` | ESP-NOW transport layer — send, receive ring buffer, callbacks |
| `display.h` | SSD1306 OLED screen manager — 5 screen modes + animations |

## Build Environments

See the root [`platformio.ini`](../platformio.ini) for `morselink_a` and
`morselink_b` build targets.

## Flash Size

Typical compiled size: **~420 KB** of flash, **~38 KB** of RAM (includes
mbedTLS, Wi-Fi stack, and Adafruit graphics library).
