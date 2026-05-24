# MorseLink — Wiring Diagram & Pin Reference

> ⚠️ **Supply voltage:** The ESP32 runs at **3.3 V** logic. The SSD1306 OLED
> accepts 3.3 V on VCC. Buttons are wired active-LOW using the MCU's internal
> pull-up resistors — no external resistors needed for them.

---

## Pin-Out Table

| Signal            | ESP32 GPIO | Connected To                          | Notes                          |
|-------------------|-----------|---------------------------------------|--------------------------------|
| I²C SDA           | GPIO 21   | OLED pin SDA                          | Built-in pull-up ~4.7 kΩ       |
| I²C SCL           | GPIO 22   | OLED pin SCL                          | Built-in pull-up ~4.7 kΩ       |
| Button — Dot      | GPIO 12   | One leg of tactile switch → GND       | INPUT_PULLUP, active-LOW       |
| Button — Dash     | GPIO 14   | One leg of tactile switch → GND       | INPUT_PULLUP, active-LOW       |
| Button — Send     | GPIO 27   | One leg of tactile switch → GND       | INPUT_PULLUP, active-LOW       |
| Button — Clear    | GPIO 26   | One leg of tactile switch → GND       | INPUT_PULLUP, active-LOW       |
| Buzzer (+)        | GPIO 25   | Passive buzzer positive leg           | 100 Ω series resistor; set -1 to disable |
| TX indicator LED  | GPIO 2    | Built-in LED (no wiring needed)       | Active-HIGH, built in          |
| 3.3 V             | 3V3 pin   | OLED VCC                              | —                              |
| Ground            | GND       | OLED GND · Buzzer GND · Button common | Common ground rail             |

---

## ASCII Schematic

```
                        ESP32 DevKit V1
                  ┌─────────────────────────┐
             3V3 ─┤ 3V3             GPIO 12 ├──[DOT BTN]──┐
             GND ─┤ GND             GPIO 14 ├──[DSH BTN]──┤
                  │                 GPIO 27 ├──[SND BTN]──┤
                  │                 GPIO 26 ├──[CLR BTN]──┘
                  │                                       │
      OLED VCC ◄──┤ 3V3                                  GND
      OLED GND ◄──┤ GND
      OLED SDA ◄──┤ GPIO 21  (I²C SDA)
      OLED SCL ◄──┤ GPIO 22  (I²C SCL)
                  │
                  │                 GPIO 25 ├──[100Ω]──[BUZZER(+)]──GND
                  │
                  │ (Built-in)      GPIO 2  ├──[LED] (TX indicator)
                  └─────────────────────────┘
```

---

## SSD1306 OLED Pinout

```
SSD1306 Module (4-pin I²C version)
┌─────┬─────┬─────┬─────┐
│ GND │ VCC │ SCL │ SDA │
└──┬──┴──┬──┴──┬──┴──┬──┘
   │     │     │     │
  GND   3V3  GPIO22 GPIO21
```

> Some 4-pin OLED modules have VCC and GND swapped. Always verify with your
> module's datasheet before powering on.

---

## Push-Button Wiring (all 4 identical)

```
 ESP32 GPIO pin ──────┤>── Tactile Switch ──── GND
                            (normally open)
```

The internal 45 kΩ pull-up keeps the pin HIGH at rest.
Pressing the button pulls it to GND (LOW) — detected as "pressed".

---

## Passive Buzzer (optional)

```
 GPIO 25 ──[ 100 Ω ]──[+BUZZER-]── GND
```

The 100 Ω series resistor limits peak current.
If you have an **active** buzzer (two-pin, internal oscillator), use any
GPIO and `digitalWrite()` instead of `tone()` — comment out the `tone()`
calls in `morse_transceiver.ino` accordingly.

---

## Power Supply

| Option            | Details                                                        |
|-------------------|----------------------------------------------------------------|
| USB (development) | Micro-USB or USB-C on the DevKit board; 500 mA from PC port   |
| Li-Po battery     | 3.7 V → boost to 5 V → USB input, or direct to VIN (5 V rail) |
| 18650 cell + TP4056 | 5 V output → VIN pin; adds charge management                |

> Typical current draw: ~80 mA idle (radio on), ~140 mA peak during TX.

---

## Component Bill of Materials

| Qty | Component                    | Typical Cost (USD) |
|-----|------------------------------|--------------------|
| 2   | ESP32 DevKit V1 (38-pin)     | $4–7 each          |
| 2   | SSD1306 OLED 128×64 (I²C)    | $2–4 each          |
| 8   | 6×6 mm tactile push switches | < $0.20 each       |
| 2   | Passive buzzer 5V            | < $0.50 each       |
| 2   | 100 Ω resistors (1/4 W)      | < $0.10 each       |
| 2   | Breadboard (half-size)       | $2–4 each          |
| 1   | Jumper wire set              | $3–6               |

**Total per pair: ~$20–$40 USD**
