/**
 * @file    display.h
 * @brief   SSD1306 128×64 OLED display manager — screens, animations, status bar.
 *
 * Screen layout (128 × 64 px):
 *
 *   ┌────────────────────────────────────────────────────────────────┐
 *   │ Status bar  (0–10 px)  — device name  |  uptime               │
 *   ├────────────────────────────────────────────────────────────────┤
 *   │ Main content area  (11–53 px)                                  │
 *   ├────────────────────────────────────────────────────────────────┤
 *   │ Help / hint bar  (54–63 px)                                    │
 *   └────────────────────────────────────────────────────────────────┘
 *
 * All functions are static — include this header once in the main .ino.
 */
#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "morse_code.h"

// The display object is defined in morse_transceiver.ino
extern Adafruit_SSD1306 display;

// Application-level state shared between .ino and display.h
extern char  morseBuf[];   // Current dot/dash sequence being entered
extern char  msgBuf[];     // Composed outgoing message
extern char  rxMsg[];      // Last received message text

// ─────────────────────────────────────────────────────────────────────────────
//  Screen identifiers
// ─────────────────────────────────────────────────────────────────────────────
enum ScreenMode : uint8_t {
    SCREEN_BOOT,
    SCREEN_IDLE,
    SCREEN_INPUT,
    SCREEN_SENDING,
    SCREEN_RECEIVED,
    SCREEN_ERROR,
};

// ─────────────────────────────────────────────────────────────────────────────
//  Boot animation — called once in setup(); blocks for ~2 s
// ─────────────────────────────────────────────────────────────────────────────
static void drawBootScreen() {
    display.clearDisplay();

    // ── Title ──
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(4, 2);
    display.print("MorseLink");

    // ── Subtitle ──
    display.setTextSize(1);
    display.setCursor(22, 22);
    display.print("ESP32 Transceiver");

    // ── AES badge ──
    display.drawRect(26, 33, 76, 11, SSD1306_WHITE);
    display.setCursor(29, 35);
    display.print("AES-128 / ESP-NOW");

    // ── Progress bar ──
    display.drawRect(14, 52, 100, 8, SSD1306_WHITE);
    display.display();

    for (int i = 0; i <= 96; i += 3) {
        display.fillRect(15, 53, i, 6, SSD1306_WHITE);
        display.display();
        delay(15);
    }
    delay(400);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Status bar  (top 10 px, always drawn first)
// ─────────────────────────────────────────────────────────────────────────────
static void drawStatusBar() {
    display.fillRect(0, 0, OLED_WIDTH, 11, SSD1306_BLACK);
    display.drawLine(0, 11, OLED_WIDTH - 1, 11, SSD1306_WHITE);

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(1, 2);
    display.print(DEVICE_NAME);

    // Uptime (right-aligned)
    char upStr[10];
    unsigned long s = millis() / 1000UL;
    if (s < 3600)       snprintf(upStr, sizeof(upStr), "%lum%02lus", s/60, s%60);
    else                snprintf(upStr, sizeof(upStr), "%luh%02lum", s/3600, (s%3600)/60);

    int16_t x1, y1; uint16_t w, h;
    display.getTextBounds(upStr, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(OLED_WIDTH - (int)w - 1, 2);
    display.print(upStr);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Idle screen — animated radio-wave rings
// ─────────────────────────────────────────────────────────────────────────────
static void drawIdleScreen(unsigned long now) {
    display.clearDisplay();
    drawStatusBar();

    const int cx = OLED_WIDTH / 2;
    const int cy = 35;

    // Antenna icon: vertical line + small horizontal bar
    display.drawLine(cx, cy - 10, cx, cy,     SSD1306_WHITE);
    display.drawLine(cx - 4, cy - 10, cx + 4, cy - 10, SSD1306_WHITE);

    // Concentric expanding rings
    uint32_t phase = (now / 350UL) % 50UL;
    for (int ring = 0; ring < 3; ++ring) {
        int r = (int)((phase + ring * 17UL) % 50UL);
        if (r > 2 && r < 24) {
            display.drawCircle(cx, cy, r, SSD1306_WHITE);
        }
    }

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(46, 52);
    display.print("STANDBY");

    display.display();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Input screen — shows current morse pattern, decoded letter, composed message
// ─────────────────────────────────────────────────────────────────────────────
static void drawInputScreen(unsigned long now) {
    display.clearDisplay();
    drawStatusBar();

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // ── Morse pattern row ──
    display.setCursor(0, 14);
    display.print("IN: ");

    // Draw dot/dash symbols as graphical glyphs
    int sx = 24;
    for (int i = 0; morseBuf[i] != '\0'; ++i) {
        if (morseBuf[i] == '.') {
            display.fillCircle(sx + 2, 17, 2, SSD1306_WHITE);
            sx += 8;
        } else {
            display.fillRect(sx, 16, 8, 3, SSD1306_WHITE);
            sx += 12;
        }
    }

    // ── Decoded letter (large font, right side) ──
    if (morseBuf[0] != '\0') {
        char dec = morseToChar(morseBuf);
        display.setTextSize(3);
        int lx = (dec == '?') ? 100 : 102;
        display.setCursor(lx, 13);
        display.print(dec);
        display.setTextSize(1);
    }

    // ── Divider ──
    display.drawLine(0, 36, OLED_WIDTH - 1, 36, SSD1306_WHITE);

    // ── Composed message ──
    display.setCursor(0, 39);
    display.print("MSG: ");

    // Scroll so the last ~18 chars are always visible
    int msgLen = (int)strlen(msgBuf);
    const char* tail = (msgLen > 18) ? msgBuf + msgLen - 18 : msgBuf;
    display.print(tail);

    // Blinking cursor
    if ((now / 500) % 2 == 0) {
        int curX = 30 + (int)strlen(tail) * 6;
        if (curX < OLED_WIDTH - 4) display.fillRect(curX, 39, 3, 7, SSD1306_WHITE);
    }

    // ── Hint bar ──
    display.setCursor(0, 55);
    display.print("[.]=dot [-]=dash [>]=send");

    display.display();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Sending screen — expanding pulse animation
// ─────────────────────────────────────────────────────────────────────────────
static void drawSendingScreen(unsigned long now) {
    display.clearDisplay();
    drawStatusBar();

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(22, 14);
    display.print("TRANSMITTING...");

    // Invert blink
    bool inv = ((now / INVERT_BLINK_MS) % 2 == 0);
    display.fillCircle(OLED_WIDTH / 2, 36, 4, inv ? SSD1306_WHITE : SSD1306_BLACK);
    if (!inv) display.drawCircle(OLED_WIDTH / 2, 36, 4, SSD1306_WHITE);

    // Propagating rings
    uint32_t ph = (now / 80UL) % 32UL;
    for (int i = 0; i < 4; ++i) {
        int r = (int)((ph + i * 8) % 32);
        if (r > 4) display.drawCircle(OLED_WIDTH / 2, 36, r, SSD1306_WHITE);
    }

    // Preview of the message being sent (truncated)
    display.setCursor(0, 55);
    int ml = (int)strlen(msgBuf);
    const char* t = (ml > 21) ? msgBuf + ml - 21 : msgBuf;
    display.print(t);

    display.display();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Received screen — shows the incoming message with a border
// ─────────────────────────────────────────────────────────────────────────────
static void drawReceivedScreen(unsigned long now) {
    display.clearDisplay();
    drawStatusBar();

    // ── Header ──
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.fillRect(0, 13, OLED_WIDTH, 9, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setCursor(10, 14);
    display.print(">> INCOMING MESSAGE");
    display.setTextColor(SSD1306_WHITE);

    // ── Message text (word-wrap 2 lines × 21 chars) ──
    int msgLen = (int)strlen(rxMsg);
    display.setCursor(0, 26);
    if (msgLen <= 21) {
        display.println(rxMsg);
    } else {
        char l1[22], l2[22];
        strncpy(l1, rxMsg,      21); l1[21] = '\0';
        strncpy(l2, rxMsg + 21, 21); l2[21] = '\0';
        display.println(l1);
        display.println(l2);
    }

    // ── Pulsing border ──
    bool pulse = ((now / 600) % 2 == 0);
    if (pulse) display.drawRect(0, 13, OLED_WIDTH, 40, SSD1306_WHITE);

    // ── Footer hint ──
    display.drawLine(0, 54, OLED_WIDTH - 1, 54, SSD1306_WHITE);
    display.setCursor(20, 56);
    display.print("[CLR] to dismiss");

    display.display();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Error screen
// ─────────────────────────────────────────────────────────────────────────────
static void displayError(const char* msg) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("!! ERROR !!");
    display.setCursor(0, 12);
    display.print(msg);
    display.display();
}

// ─────────────────────────────────────────────────────────────────────────────
//  updateDisplay  — call every loop(); throttles to DISPLAY_FPS
// ─────────────────────────────────────────────────────────────────────────────
static void updateDisplay(ScreenMode screen, unsigned long now) {
    static unsigned long lastRefresh = 0;
    if (now - lastRefresh < (1000UL / DISPLAY_FPS)) return;
    lastRefresh = now;

    switch (screen) {
        case SCREEN_IDLE:     drawIdleScreen(now);     break;
        case SCREEN_INPUT:    drawInputScreen(now);    break;
        case SCREEN_SENDING:  drawSendingScreen(now);  break;
        case SCREEN_RECEIVED: drawReceivedScreen(now); break;
        default: break;
    }
}
