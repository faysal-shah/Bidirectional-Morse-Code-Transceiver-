/**
 * ╔══════════════════════════════════════════════════════════════════╗
 * ║              M O R S E L I N K   v1.0                           ║
 * ║   Bidirectional Encrypted Morse Code Transceiver — ESP32        ║
 * ╠══════════════════════════════════════════════════════════════════╣
 * ║  Author  : Faysal Shah                                          ║
 * ║  Hardware: ESP32 DevKit · SSD1306 128×64 OLED · 4 push-buttons ║
 * ║  Radio   : ESP-NOW (IEEE 802.11)  ·  No Wi-Fi router needed     ║
 * ║  Crypto  : AES-128-CBC + HMAC-SHA256  (mbedTLS, hardware-aided) ║
 * ╚══════════════════════════════════════════════════════════════════╝
 *
 * How to use
 * ──────────
 *  1. Tap [DOT] quickly  → adds a dot   (·)
 *  2. Hold [DOT] > 300ms → adds a dash  (–)    ← two-button OR one-button mode
 *     OR tap [DASH] any time → always a dash
 *  3. Wait ~0.9 s after last symbol  → letter is decoded and appended to message
 *  4. Wait ~2.5 s                    → a space is inserted between words
 *  5. Press [SEND]  → encrypts and transmits the message via ESP-NOW
 *  6. Press [CLEAR] → wipe current input without sending
 *
 * Required libraries (Arduino Library Manager)
 * ─────────────────────────────────────────────
 *   • Adafruit SSD1306  (v2.5+)
 *   • Adafruit GFX Library (v1.11+)
 *   — All others are part of the ESP32 Arduino core —
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_now.h>
#include <WiFi.h>
#include "mbedtls/aes.h"
#include "mbedtls/md.h"

#include "config.h"
#include "morse_code.h"
#include "crypto.h"
#include "comm.h"
#include "display.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Global display object  (declared extern in display.h)
// ─────────────────────────────────────────────────────────────────────────────
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

// ─────────────────────────────────────────────────────────────────────────────
//  Shared string buffers  (declared extern in display.h)
// ─────────────────────────────────────────────────────────────────────────────
char morseBuf[MAX_MORSE_SYMBOLS + 1] = "";  // Current dot/dash sequence
char msgBuf[MAX_MSG_CHARS   + 1] = "";      // Composed outgoing message
char rxMsg [MAX_MSG_CHARS   + 1] = "";      // Last received message

// ─────────────────────────────────────────────────────────────────────────────
//  Input state machine
// ─────────────────────────────────────────────────────────────────────────────
static int          morsePos        = 0;
static int          msgPos          = 0;
static unsigned long lastKeyTime    = 0;
static bool         letterPending   = false;   // true = symbols entered, not committed
static bool         wordGapDone     = false;   // prevent repeated space insertions

// ─────────────────────────────────────────────────────────────────────────────
//  Button state trackers
// ─────────────────────────────────────────────────────────────────────────────
static bool          dotDown        = false;
static unsigned long dotDownAt      = 0;
static bool          dashDown       = false;
static unsigned long dashDownAt     = 0;

// ─────────────────────────────────────────────────────────────────────────────
//  UI state
// ─────────────────────────────────────────────────────────────────────────────
static ScreenMode   screen          = SCREEN_BOOT;
static unsigned long rxDisplayUntil = 0;
static bool         hasNewMsg       = false;

// ─────────────────────────────────────────────────────────────────────────────
//  Helper: beep  (no-op if PIN_BUZZER == -1)
// ─────────────────────────────────────────────────────────────────────────────
static void beep(int freqHz, int durationMs) {
    if (PIN_BUZZER < 0) return;
    tone(PIN_BUZZER, freqHz, durationMs);
}

// ─────────────────────────────────────────────────────────────────────────────
//  appendMorse  — add one symbol to morseBuf
// ─────────────────────────────────────────────────────────────────────────────
static void appendMorse(char sym) {
    if (morsePos >= MAX_MORSE_SYMBOLS) return;
    morseBuf[morsePos++] = sym;
    morseBuf[morsePos]   = '\0';
    letterPending  = true;
    wordGapDone    = false;
    screen         = SCREEN_INPUT;
    Serial.printf("[MORSE] %c  → \"%s\"\n", sym, morseBuf);
}

// ─────────────────────────────────────────────────────────────────────────────
//  commitLetter  — decode morseBuf and append the character to msgBuf
// ─────────────────────────────────────────────────────────────────────────────
static void commitLetter() {
    if (morsePos == 0) return;

    char ch = morseToChar(morseBuf);
    Serial.printf("[MORSE] Commit \"%s\" → '%c'\n", morseBuf, ch);

    if (ch != '?' && msgPos < MAX_MSG_CHARS) {
        msgBuf[msgPos++] = ch;
        msgBuf[msgPos]   = '\0';
        beep(900, 60);
    }

    // Reset morse buffer
    morsePos    = 0;
    morseBuf[0] = '\0';
    letterPending = false;
}

// ─────────────────────────────────────────────────────────────────────────────
//  sendMessage  — encrypt and transmit, then clear buffers
// ─────────────────────────────────────────────────────────────────────────────
static void sendMessage() {
    // Commit any pending last character
    if (morsePos > 0) commitLetter();

    // Trim trailing spaces
    while (msgPos > 0 && msgBuf[msgPos - 1] == ' ') msgBuf[--msgPos] = '\0';

    if (msgPos == 0) {
        Serial.println("[TX] Nothing to send");
        return;
    }

    screen = SCREEN_SENDING;
    updateDisplay(screen, millis());

    Serial.printf("[TX] → \"%s\"\n", msgBuf);
    bool ok = commSendMessage(msgBuf);

    if (ok) {
        beep(1200, 80); delay(110); beep(1600, 120);
        Serial.println("[TX] ✓ Accepted by ESP-NOW");
    } else {
        beep(200, 600);
        Serial.println("[TX] ✗ Send failed");
    }

    // Clear after send regardless of outcome
    msgBuf[0]   = '\0'; msgPos      = 0;
    morseBuf[0] = '\0'; morsePos    = 0;
    letterPending  = false;
    wordGapDone    = false;
    screen = SCREEN_IDLE;
}

// ─────────────────────────────────────────────────────────────────────────────
//  clearInput  — wipe everything
// ─────────────────────────────────────────────────────────────────────────────
static void clearInput() {
    msgBuf[0]   = '\0'; msgPos      = 0;
    morseBuf[0] = '\0'; morsePos    = 0;
    letterPending  = false;
    wordGapDone    = false;
    if (!hasNewMsg) screen = SCREEN_IDLE;
    beep(300, 80);
    Serial.println("[INPUT] Cleared");
}

// ─────────────────────────────────────────────────────────────────────────────
//  setup
// ─────────────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n\n==== MorseLink v1.0 boot ====");
    Serial.printf("Device ID   : %d\n", DEVICE_ID);
    Serial.printf("Device Name : %s\n", DEVICE_NAME);
    Serial.printf("ESP-NOW Ch  : %d\n", ESPNOW_CHANNEL);

    // ── Pins ──
    pinMode(PIN_BTN_DOT,   INPUT_PULLUP);
    pinMode(PIN_BTN_DASH,  INPUT_PULLUP);
    pinMode(PIN_BTN_SEND,  INPUT_PULLUP);
    pinMode(PIN_BTN_CLEAR, INPUT_PULLUP);
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);

    if (PIN_BUZZER >= 0) {
        pinMode(PIN_BUZZER, OUTPUT);
        // Startup chime
        beep(600, 80); delay(100);
        beep(900, 80); delay(100);
        beep(1200, 100);
    }

    // ── OLED ──
    Wire.begin(OLED_SDA, OLED_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("FATAL: OLED not found! Check wiring / I2C address.");
        // Blink LED forever to signal the fault
        while (true) {
            digitalWrite(PIN_LED, !digitalRead(PIN_LED));
            delay(200);
        }
    }
    display.clearDisplay();
    display.display();

    drawBootScreen();   // ~2 s animation

    // ── ESP-NOW ──
    if (!commInit()) {
        displayError("ESP-NOW failed\nCheck antenna");
        while (true) delay(1000);
    }

    screen = SCREEN_IDLE;
    Serial.println("==== Boot complete — ready ====\n");
}

// ─────────────────────────────────────────────────────────────────────────────
//  loop
// ─────────────────────────────────────────────────────────────────────────────
void loop() {
    unsigned long now = millis();

    // ── DOT button (dual-function: short = dot, long = dash) ──────────────
    bool dotNow = (digitalRead(PIN_BTN_DOT) == LOW);
    if (dotNow && !dotDown) {
        dotDown   = true;
        dotDownAt = now;
    } else if (!dotNow && dotDown) {
        unsigned long held = now - dotDownAt;
        dotDown = false;
        if (held >= DEBOUNCE_MS) {
            appendMorse(held <= MORSE_DOT_MAX ? '.' : '-');
            lastKeyTime = now;
            beep(held <= MORSE_DOT_MAX ? 900 : 600,
                 held <= MORSE_DOT_MAX ?  60 : 180);
        }
    }

    // ── DASH button (always a dash) ────────────────────────────────────────
    bool dashNow = (digitalRead(PIN_BTN_DASH) == LOW);
    if (dashNow && !dashDown) {
        dashDown   = true;
        dashDownAt = now;
    } else if (!dashNow && dashDown) {
        unsigned long held = now - dashDownAt;
        dashDown = false;
        if (held >= DEBOUNCE_MS) {
            appendMorse('-');
            lastKeyTime = now;
            beep(600, 180);
        }
    }

    // ── Automatic letter commit ────────────────────────────────────────────
    if (letterPending && morsePos > 0 &&
        (now - lastKeyTime) >= MORSE_LETTER_GAP) {
        commitLetter();
        lastKeyTime = now;   // Reset so word-gap timer starts now
    }

    // ── Automatic word-space insertion ─────────────────────────────────────
    if (!letterPending && !wordGapDone && msgPos > 0 &&
        msgBuf[msgPos - 1] != ' ' &&
        (now - lastKeyTime) >= MORSE_WORD_GAP) {
        if (msgPos < MAX_MSG_CHARS) {
            msgBuf[msgPos++] = ' ';
            msgBuf[msgPos]   = '\0';
            wordGapDone = true;
            Serial.println("[MORSE] Word space");
        }
    }

    // ── SEND button ────────────────────────────────────────────────────────
    static bool sendPrev = false;
    bool sendNow = (digitalRead(PIN_BTN_SEND) == LOW);
    if (sendNow && !sendPrev) sendMessage();
    sendPrev = sendNow;

    // ── CLEAR button ───────────────────────────────────────────────────────
    static bool clrPrev = false;
    bool clrNow = (digitalRead(PIN_BTN_CLEAR) == LOW);
    if (clrNow && !clrPrev) {
        if (hasNewMsg) {
            // Dismiss received message
            hasNewMsg = false;
            screen    = (msgPos > 0) ? SCREEN_INPUT : SCREEN_IDLE;
        } else {
            clearInput();
        }
    }
    clrPrev = clrNow;

    // ── Receive polling ────────────────────────────────────────────────────
    {
        char rxBuf[MAX_MSG_CHARS + 1];
        uint8_t srcId = 0;
        if (commReceive(rxBuf, sizeof(rxBuf), &srcId)) {
            strncpy(rxMsg, rxBuf, MAX_MSG_CHARS);
            rxMsg[MAX_MSG_CHARS] = '\0';
            hasNewMsg      = true;
            rxDisplayUntil = now + RX_DISPLAY_MS;
            screen         = SCREEN_RECEIVED;
            // Incoming chime: two ascending beeps
            beep(600, 80); delay(120); beep(1000, 150);
            // Also print decoded Morse to Serial for debugging
            char morseLine[256];
            textToMorse(rxMsg, morseLine, sizeof(morseLine));
            Serial.printf("[RX] Text  : \"%s\"\n", rxMsg);
            Serial.printf("[RX] Morse : %s\n", morseLine);
        }
    }

    // ── RX message display timeout ─────────────────────────────────────────
    if (hasNewMsg && now >= rxDisplayUntil) {
        hasNewMsg = false;
        screen    = (msgPos > 0) ? SCREEN_INPUT : SCREEN_IDLE;
    }

    // ── Update display ─────────────────────────────────────────────────────
    updateDisplay(screen, now);
}
