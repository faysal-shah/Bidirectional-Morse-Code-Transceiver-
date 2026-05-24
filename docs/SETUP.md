# MorseLink — Setup Guide

This guide walks you from unboxing components to two devices exchanging
encrypted messages.

---

## Prerequisites

### Hardware (per device)

- **ESP32 DevKit V1** (38-pin, any USB-serial variant)
- **SSD1306 OLED 128×64** (I²C, 4-pin: GND/VCC/SCL/SDA)
- **4 × 6mm tactile push buttons**
- 1 × passive buzzer (optional)
- 1 × 100 Ω resistor (for buzzer)
- Breadboard + jumper wires

### Software

| Tool | Version | Install |
|------|---------|---------|
| [Arduino IDE](https://www.arduino.cc/en/software) | 2.x+ | arduino.cc |
| ESP32 Arduino Core | 2.0.14+ | Board manager → "esp32 by Espressif" |
| Adafruit SSD1306 | 2.5.7+ | Library manager |
| Adafruit GFX Library | 1.11.5+ | Library manager |

> **Alternative:** [PlatformIO](https://platformio.org/) (VS Code extension).
> Dependencies are declared in `platformio.ini` and installed automatically.

---

## Step 1 — Wire the Hardware

Follow the table in [`schematics/wiring_diagram.md`](../schematics/wiring_diagram.md)
or refer to the visual schematic at [`assets/schematic.svg`](../assets/schematic.svg).

Key points:
- OLED: 3.3 V supply, I²C on GPIO 21 (SDA) and GPIO 22 (SCL).
- Buttons: wire one leg to the GPIO, other leg to GND. No resistor needed.
- Double-check OLED orientation — some modules have VCC and GND swapped.

---

## Step 2 — Install Libraries (Arduino IDE)

1. Open Arduino IDE → **Tools → Manage Libraries…**
2. Search for **"Adafruit SSD1306"** → Install (installs GFX automatically).
3. Install **ESP32 board support** if not present:
   - **File → Preferences** → add to Additional boards URL:
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - **Tools → Board → Boards Manager** → search "esp32" → Install.

---

## Step 3 — Open the Sketch

```
firmware/morse_transceiver/morse_transceiver.ino
```

Open this file in Arduino IDE (it will also load the `.h` files from the same
folder automatically).

---

## Step 4 — Configure Device A

In **`config.h`**, verify or update:

```cpp
#define DEVICE_ID    0               // Unit A
#define DEVICE_NAME  "MorseLink-A"
#define AES_KEY      "MorseLink-K3y!!"  // Change this!
#define HMAC_KEY     "MorseHMAC-K3y!!"  // Change this!
```

Select board:
- **Tools → Board → ESP32 Dev Module**
- **Tools → Port → (your COM port)**
- **Upload Speed → 921600**

Flash Device A.

---

## Step 5 — Find the MAC Addresses

1. Open **Tools → Serial Monitor** (baud 115200).
2. Reset Device A. Look for:
   ```
   [COMM] MAC → AA:BB:CC:DD:EE:FF
   ```
3. Do the same for Device B after flashing (Step 6).
4. Note both MAC addresses.

---

## Step 6 — Configure and Flash Device B

Edit `config.h`:

```cpp
#define DEVICE_ID    1              // Unit B
#define DEVICE_NAME  "MorseLink-B"
// Same AES_KEY and HMAC_KEY as Device A!
```

Flash Device B with these settings.

---

## Step 7 — Pair the Devices (Recommended)

For private communication, update the peer MAC on each device:

**Device A's config.h:**
```cpp
// Replace with Device B's actual MAC
#define PEER_MAC_ADDR  { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF }
```

**Device B's config.h:**
```cpp
// Replace with Device A's actual MAC
#define PEER_MAC_ADDR  { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 }
```

Re-flash both devices. They will now only communicate with each other.

---

## Step 8 — Test

1. Power on both devices. Each shows the boot animation and then **STANDBY**.
2. On Device A, tap [DOT] twice → wait → the display shows `I`.
3. Continue tapping out **"HI"** → press [SEND].
4. Device B should display `>> INCOMING MESSAGE: HI`.

---

## Troubleshooting

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| OLED blank after boot | Wrong I²C address or SDA/SCL swapped | Try `0x3D` in `OLED_ADDR`; check wiring |
| LED fast-blinks, no OLED output | OLED not detected (halted in setup) | Check VCC/GND orientation |
| Messages not received | Different channel or AES keys | Ensure both devices have the same `ESPNOW_CHANNEL`, `AES_KEY`, `HMAC_KEY` |
| `[CRYPTO] HMAC mismatch` in Serial | Keys don't match or packet corrupted | Verify `AES_KEY` and `HMAC_KEY` are identical on both units |
| Buttons feel laggy | Debounce too aggressive | Reduce `DEBOUNCE_MS` in `config.h` |
| Dot/dash mis-detected | Threshold wrong for your buttons | Adjust `MORSE_DOT_MAX` |
| No buzzer sound | `PIN_BUZZER` wrong or active buzzer | Set `PIN_BUZZER -1` and use `digitalWrite` instead of `tone` |

---

## Using PlatformIO (Alternative)

```bash
# Clone the repo
git clone https://github.com/faysal-shah/bidirectional-morse-code-transceiver-.git
cd bidirectional-morse-code-transceiver-

# Flash Device A
pio run -e morselink_a -t upload

# Flash Device B
pio run -e morselink_b -t upload

# Monitor serial output
pio device monitor -e morselink_a
```
