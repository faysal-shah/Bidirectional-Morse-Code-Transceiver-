<div align="center">

```
███╗   ███╗ ██████╗ ██████╗ ███████╗███████╗██╗     ██╗███╗   ██╗██╗  ██╗
████╗ ████║██╔═══██╗██╔══██╗██╔════╝██╔════╝██║     ██║████╗  ██║██║ ██╔╝
██╔████╔██║██║   ██║██████╔╝███████╗█████╗  ██║     ██║██╔██╗ ██║█████╔╝
██║╚██╔╝██║██║   ██║██╔══██╗╚════██║██╔══╝  ██║     ██║██║╚██╗██║██╔═██╗
██║ ╚═╝ ██║╚██████╔╝██║  ██║███████║███████╗███████╗██║██║ ╚████║██║  ██╗
╚═╝     ╚═╝ ╚═════╝ ╚═╝  ╚═╝╚══════╝╚══════╝╚══════╝╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝
```

### Bidirectional Encrypted Morse Code Transceiver

**ESP32 · SSD1306 OLED · ESP-NOW · AES-128-CBC + HMAC-SHA256**

[![Platform](https://img.shields.io/badge/Platform-ESP32-blue?logo=espressif&logoColor=white)](https://www.espressif.com/en/products/socs/esp32)
[![Framework](https://img.shields.io/badge/Framework-Arduino-teal?logo=arduino&logoColor=white)](https://www.arduino.cc/)
[![Protocol](https://img.shields.io/badge/Radio-ESP--NOW-orange)](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/network/esp_now.html)
[![Encryption](https://img.shields.io/badge/Crypto-AES--128--CBC-red?logo=lockOpen)](https://en.wikipedia.org/wiki/Advanced_Encryption_Standard)
[![License](https://img.shields.io/badge/License-MIT-green)](LICENSE)
[![Language](https://img.shields.io/badge/Language-C%2B%2B17-purple?logo=cplusplus)](https://en.cppreference.com/)

</div>

---

## 📡 What Is MorseLink?

**MorseLink** is a peer-to-peer wireless communicator that lets two people
exchange **encrypted text messages using Morse code input** — no smartphones,
no internet, no base station required.

Each unit is an **ESP32** microcontroller paired with a **128×64 OLED display**
and four tactile buttons. You tap dots and dashes; the device decodes them in
real time, builds the message, encrypts it with **AES-128-CBC + HMAC-SHA256**,
and fires it over the 2.4 GHz band via **ESP-NOW** — reaching the partner
device up to ~200 m away with sub-10 ms latency.

```
  Unit A                                         Unit B
┌────────────────────┐    ESP-NOW (Wi-Fi PHY)  ┌────────────────────┐
│  · − · ·           │  ~~~~~~~~~~~~~~~~~~~►   │  >> INCOMING MSG   │
│  MORSE: .-..       │  AES-128 / HMAC-SHA256  │  "LETS MEET"       │
│  MSG: LETS MEET    │  ◄~~~~~~~~~~~~~~~~~~~   │                    │
│  [·][−][▶][✕]      │                         │  [·][−][▶][✕]      │
└────────────────────┘                         └────────────────────┘
     OLED + 4 buttons                               OLED + 4 buttons
```

---

## ✨ Features

| Category | Detail |
|---|---|
| 🔐 **Encryption** | AES-128-CBC with per-message random IV; HMAC-SHA256 Encrypt-then-MAC |
| 📻 **Wireless** | ESP-NOW over 802.11 — no router, no pairing handshake, < 10 ms latency |
| 📺 **Display** | Animated SSD1306 128×64 OLED — boot screen, idle radio-wave, input, send/receive |
| ⌨️ **Input** | 4-button interface; [DOT] dual-function (short=`·`, long=`−`); auto letter-commit |
| 🔔 **Feedback** | Buzzer chimes for sent/received; built-in LED blinks on TX |
| 📦 **Portable** | ~80 mA idle; runs on 3.7 V Li-Po + TP4056 charger |
| 🧰 **Toolchain** | Arduino IDE 2.x **or** PlatformIO; mbedTLS built into ESP32 core |
| 🔧 **Configurable** | Single `config.h` — keys, pins, timing, device ID |

---

## 🛠 Hardware

### Bill of Materials (per pair of units)

| Qty | Component | Approx. Cost |
|-----|-----------|-------------|
| 2 | [ESP32 DevKit V1](https://www.espressif.com/en/products/devkits) (38-pin) | $8–14 |
| 2 | SSD1306 OLED 128×64 I²C (4-pin) | $4–8 |
| 8 | 6×6 mm tactile push buttons | $1–2 |
| 2 | Passive buzzer (5 V) | $1 |
| 2 | 100 Ω resistors (¼ W) | < $1 |
| 2 | Half-size breadboard | $4–8 |
| 1 | Jumper wire set | $3–6 |
| | **Total** | **~$21–$40** |

---

## 🔌 Pin Wiring

```
                        ┌─────────────────────────┐
            OLED VCC ◄──┤ 3V3             GPIO 12 ├──► [DOT  Button] ──┐
            OLED GND ◄──┤ GND             GPIO 14 ├──► [DASH Button] ──┤
            OLED SDA ◄──┤ GPIO 21  SDA    GPIO 27 ├──► [SEND Button] ──┤
            OLED SCL ◄──┤ GPIO 22  SCL    GPIO 26 ├──► [CLEAR Button]──┘
                        │                                               │
                        │                 GPIO 25 ├──[100Ω]──[BUZZER] GND
                        │  ESP32          GPIO 2  ├──► Built-in LED
                        └─────────────────────────┘
                                                   └── All buttons → GND
```

> Full wiring table and ASCII schematic: [`schematics/wiring_diagram.md`](schematics/wiring_diagram.md)  
> Visual circuit diagram (SVG): [`assets/schematic.svg`](assets/schematic.svg)

---

## 🚀 Quick Start

### Option A — Arduino IDE

1. **Install the ESP32 board package**
   - File → Preferences → add board manager URL:
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Tools → Boards Manager → install **esp32 by Espressif Systems**

2. **Install libraries** (Tools → Manage Libraries)
   - `Adafruit SSD1306`
   - `Adafruit GFX Library`

3. **Open sketch**
   ```
   firmware/morse_transceiver/morse_transceiver.ino
   ```

4. **Configure** `config.h` — set `DEVICE_ID`, `DEVICE_NAME`, and your own
   `AES_KEY` / `HMAC_KEY` (same values on both units).

5. **Flash** — select *ESP32 Dev Module*, correct COM port, upload.

6. Repeat steps 4–5 for the second unit with `DEVICE_ID = 1`.

### Option B — PlatformIO

```bash
git clone https://github.com/faysal-shah/bidirectional-morse-code-transceiver-.git
cd bidirectional-morse-code-transceiver-

# Flash Unit A
pio run -e morselink_a -t upload

# Flash Unit B
pio run -e morselink_b -t upload

# Open serial monitor
pio device monitor
```

---

## 🎮 How to Use

### Button Layout

```
┌─────┐  ┌─────┐  ┌─────┐  ┌─────┐
│  ·  │  │  −  │  │  ▶  │  │  ✕  │
│ DOT │  │DASH │  │SEND │  │ CLR │
└─────┘  └─────┘  └─────┘  └─────┘
GPIO 12  GPIO 14  GPIO 27  GPIO 26
```

### Morse Input

| Action | Result |
|--------|--------|
| Tap [DOT] < 300 ms | Enters a **dot** `·` |
| Hold [DOT] > 300 ms | Enters a **dash** `−` |
| Tap [DASH] (any duration) | Always enters a **dash** `−` |
| Silence for 0.9 s | **Commits** current symbols as a letter |
| Silence for 2.5 s | Inserts a **word space** |
| Press [SEND] | **Encrypts and transmits** the message |
| Press [CLEAR] | Wipes current input without sending |
| Press [CLEAR] while message showing | Dismisses received message |

### Live Example — Sending "HI"

```
[DOT] [DOT] [DOT] [DOT]  →  ....  →  H       (wait 0.9 s)
[DOT] [DOT]              →  ..    →  I       (wait 0.9 s)
[SEND]                   →  transmit "HI"
```

---

## 📟 Display Screens

| Screen | Description |
|--------|-------------|
| **Boot** | Animated logo + progress bar (2 s) |
| **Standby** | Radio-wave animation with expanding concentric rings |
| **Input** | Live Morse symbols + decoded letter (large font) + message buffer |
| **Transmitting** | Pulsing rings animation + message preview |
| **Received** | Highlighted incoming message with blinking border |

---

## 🔐 Security Architecture

```
 Plaintext: "HELLO"
      │
      ▼  AES-128-CBC
 ┌──────────┬──────────┬─────────────────┐
 │  IV 16B  │ HMAC 32B │  Ciphertext     │  ← ESP-NOW payload
 └──────────┴──────────┴─────────────────┘
      │                      ▲
      └── HMAC covers ────────┘
          IV + ciphertext
          (Encrypt-then-MAC)
```

- **AES key and HMAC key** are compile-time constants in `config.h`.  
  Change both before any real deployment.
- **IV** is freshly generated from the ESP32 hardware TRNG on every send.
- **HMAC** is verified with a constant-time comparison before decryption.
- See [`docs/PROTOCOL.md`](docs/PROTOCOL.md) for full protocol specification.

---

## 📁 Repository Structure

```
Bidirectional-Morse-Code-Transceiver-/
├── firmware/
│   └── morse_transceiver/
│       ├── morse_transceiver.ino   Main sketch
│       ├── config.h                All tunable parameters
│       ├── morse_code.h            ITU Morse encode / decode
│       ├── crypto.h                AES-128-CBC + HMAC-SHA256
│       ├── comm.h                  ESP-NOW transport layer
│       └── display.h               OLED screens & animations
├── schematics/
│   └── wiring_diagram.md           Pin table + ASCII diagram + BOM
├── docs/
│   ├── PROTOCOL.md                 Packet format & security design
│   ├── SETUP.md                    Step-by-step setup guide
│   └── MORSE_REFERENCE.md          ITU Morse code chart & timing
├── assets/
│   └── schematic.svg               Visual circuit schematic (SVG)
├── platformio.ini                  PlatformIO build config (A & B envs)
├── LICENSE
└── README.md
```

---

## 📶 Range & Performance

| Condition | Typical Range |
|-----------|---------------|
| Open air, line-of-sight | ~150–200 m |
| Indoor (same floor) | ~30–80 m |
| Through one concrete wall | ~10–30 m |
| Latency (LOS) | < 10 ms |
| Message throughput | Limited by human Morse input speed (~5 WPM typical) |

---

## 🔧 Configuration Reference

All knobs live in `firmware/morse_transceiver/config.h`:

```cpp
// ── Identity ──────────────────────────────────────────────
#define DEVICE_ID       0          // 0 = Unit A,  1 = Unit B
#define DEVICE_NAME     "MorseLink-A"

// ── Pins ──────────────────────────────────────────────────
#define PIN_BTN_DOT     12
#define PIN_BTN_DASH    14
#define PIN_BTN_SEND    27
#define PIN_BTN_CLEAR   26
#define OLED_SDA        21
#define OLED_SCL        22
#define PIN_BUZZER      25         // set -1 to disable

// ── Morse timing ──────────────────────────────────────────
#define MORSE_DOT_MAX   300        // ms; shorter press = dot
#define MORSE_LETTER_GAP 900       // ms silence → commit letter
#define MORSE_WORD_GAP  2500       // ms silence → insert space

// ── Encryption keys (change these!) ───────────────────────
#define AES_KEY         "MorseLink-K3y!!"   // exactly 16 bytes
#define HMAC_KEY        "MorseHMAC-K3y!!"  // exactly 16 bytes

// ── Wireless ──────────────────────────────────────────────
#define ESPNOW_CHANNEL  6
#define PEER_MAC_ADDR   { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } // → pair after first boot
```

---

## 🗺 Roadmap

- [ ] Persistent message log in NVS flash
- [ ] Group broadcast (more than 2 devices)
- [ ] Battery level indicator (ADC on GPIO 35 via voltage divider)
- [ ] Wi-Fi station fallback for longer range through a router
- [ ] QR-code based key exchange for easier pairing
- [ ] Hardware encryption key in ESP32 eFuse (anti-extraction)

---

## 🤝 Contributing

Contributions are welcome! Please:

1. Fork the repository and create a feature branch.
2. Follow the existing code style (clang-format, Arduino conventions).
3. Add / update documentation if the change affects wiring or usage.
4. Open a pull request with a clear description.

---

## 📄 License

Distributed under the **MIT License**. See [`LICENSE`](LICENSE) for full text.

---

## 👤 Author

**Faysal Shah** — [faysal-shah](https://github.com/faysal-shah)  
*GDP Pakistan*

---

<div align="center">

*"What hath God wrought" — first message ever sent by Morse code, 24 May 1844*

</div>
