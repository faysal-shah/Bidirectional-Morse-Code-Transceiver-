/**
 * @file    config.h
 * @brief   MorseLink — all compile-time configuration knobs in one place.
 *
 * Edit DEVICE_ID and DEVICE_NAME for each unit, then re-flash.
 * Both units must share the same AES_KEY / HMAC_KEY / ESPNOW_CHANNEL.
 */
#pragma once

// ─────────────────────────────────────────────────────────────────────────────
//  Device Identity
//  Change DEVICE_ID to 1 (and DEVICE_NAME) when flashing the second unit.
// ─────────────────────────────────────────────────────────────────────────────
#define DEVICE_ID       0               // 0 = Unit A,  1 = Unit B
#define DEVICE_NAME     "MorseLink-A"   // Max 12 chars (fits status bar)

// ─────────────────────────────────────────────────────────────────────────────
//  Pin Assignments
//  All button pins are wired active-LOW (button → GND, INPUT_PULLUP enabled).
// ─────────────────────────────────────────────────────────────────────────────
#define PIN_BTN_DOT     12   // Dot input
#define PIN_BTN_DASH    14   // Dash input
#define PIN_BTN_SEND    27   // Send composed message
#define PIN_BTN_CLEAR   26   // Clear / backspace

#define OLED_SDA        21   // I2C data
#define OLED_SCL        22   // I2C clock
#define OLED_ADDR       0x3C // SSD1306 default address (some modules use 0x3D)
#define OLED_WIDTH      128
#define OLED_HEIGHT     64

#define PIN_BUZZER      25   // Active-LOW passive buzzer; set -1 to disable
#define PIN_LED         2    // Built-in LED — blinks on TX

// ─────────────────────────────────────────────────────────────────────────────
//  Morse Code Timing  (milliseconds)
// ─────────────────────────────────────────────────────────────────────────────
#define MORSE_DOT_MAX       300   // Key held ≤ 300 ms  → dot (·)
// Key held > 300 ms via PIN_BTN_DOT, OR any press of PIN_BTN_DASH → dash (–)
#define MORSE_LETTER_GAP    900   // Silence after last symbol → commit letter
#define MORSE_WORD_GAP     2500   // Longer silence → insert space between words
#define DEBOUNCE_MS         40    // Button debounce window

// ─────────────────────────────────────────────────────────────────────────────
//  Buffer Sizes
// ─────────────────────────────────────────────────────────────────────────────
#define MAX_MSG_CHARS       120   // Max decoded characters per outgoing message
#define MAX_MORSE_SYMBOLS    10   // Max dots/dashes for a single character

// ─────────────────────────────────────────────────────────────────────────────
//  Encryption  (AES-128-CBC + HMAC-SHA256)
//  Both keys MUST be exactly 16 bytes. Change before deploying in production.
// ─────────────────────────────────────────────────────────────────────────────
#define AES_KEY         "MorseLink-K3y!!"   // 16 bytes — AES-128 session key
#define HMAC_KEY        "MorseHMAC-K3y!!"  // 16 bytes — HMAC-SHA256 auth key

// ─────────────────────────────────────────────────────────────────────────────
//  Wireless / ESP-NOW
// ─────────────────────────────────────────────────────────────────────────────
// Wi-Fi channel used by ESP-NOW (1–13; must match on both devices).
#define ESPNOW_CHANNEL  6

// Peer MAC — leave as broadcast {0xFF,…} for the first boot; after pairing
// replace with the specific MAC so only your pair can receive frames.
#define PEER_MAC_ADDR   { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }

// ─────────────────────────────────────────────────────────────────────────────
//  Protocol constants
// ─────────────────────────────────────────────────────────────────────────────
#define PROTO_MAGIC     0x4D4C  // 'ML' — rejects alien ESP-NOW traffic
#define PROTO_VERSION   1

// ─────────────────────────────────────────────────────────────────────────────
//  Display / UX tweaks
// ─────────────────────────────────────────────────────────────────────────────
#define RX_DISPLAY_MS   8000   // How long to show a received message (ms)
#define DISPLAY_FPS     30     // Target display refresh rate
