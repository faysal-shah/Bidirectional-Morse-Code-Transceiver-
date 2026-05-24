/**
 * @file    morse_code.h
 * @brief   ITU Morse Code lookup tables and encode / decode helpers.
 *
 * Supported character set: A–Z, 0–9, and common punctuation (. , ? ! / @).
 * Functions are intentionally kept simple (linear scan) — the table fits in
 * ~700 bytes of flash and lookup latency is negligible at human-input speed.
 */
#pragma once
#include <Arduino.h>

// ─────────────────────────────────────────────────────────────────────────────
//  Lookup table  — index matches MORSE_CHARS below
// ─────────────────────────────────────────────────────────────────────────────
static const char* const MORSE_TABLE[] PROGMEM = {
    // A–Z  (indices 0–25)
    ".-",    "-...",  "-.-.",  "-..",   ".",     "..-.",  "--.",   "....",
    "..",    ".---",  "-.-",   ".-..",  "--",    "-.",    "---",   ".--.",
    "--.-",  ".-.",   "...",   "-",     "..-",   "...-",  ".--",   "-..-",
    "-.--",  "--..",
    // 0–9  (indices 26–35)
    "-----", ".----", "..---", "...--", "....-",
    ".....", "-....", "--...", "---..", "----.",
    // Punctuation  (indices 36–41)
    ".-.-.-",   // .
    "--..--",   // ,
    "..--..",   // ?
    "-.-.--",   // !
    "-..-.",    // /
    ".--.-.",   // @
};

// Parallel character array (same order as MORSE_TABLE)
static const char MORSE_CHARS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?!/@";
static const int  MORSE_TABLE_LEN = sizeof(MORSE_CHARS) - 1;  // exclude NUL

// ─────────────────────────────────────────────────────────────────────────────
//  charToMorse — returns the dot/dash string for a printable ASCII character.
//  Returns nullptr for unmapped characters.
// ─────────────────────────────────────────────────────────────────────────────
static const char* charToMorse(char c) {
    c = toupper((unsigned char)c);
    if (c >= 'A' && c <= 'Z') return MORSE_TABLE[c - 'A'];
    if (c >= '0' && c <= '9') return MORSE_TABLE[26 + (c - '0')];
    switch (c) {
        case '.': return MORSE_TABLE[36];
        case ',': return MORSE_TABLE[37];
        case '?': return MORSE_TABLE[38];
        case '!': return MORSE_TABLE[39];
        case '/': return MORSE_TABLE[40];
        case '@': return MORSE_TABLE[41];
    }
    return nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
//  morseToChar — decodes a dot/dash string to its ASCII character.
//  Returns '?' if no match, ' ' for an empty/null input.
// ─────────────────────────────────────────────────────────────────────────────
static char morseToChar(const char* morse) {
    if (!morse || morse[0] == '\0') return ' ';
    for (int i = 0; i < MORSE_TABLE_LEN; ++i) {
        if (strcmp(morse, MORSE_TABLE[i]) == 0) return MORSE_CHARS[i];
    }
    return '?';
}

// ─────────────────────────────────────────────────────────────────────────────
//  textToMorse — converts a full string to a human-readable Morse sequence.
//  Letters are separated by a single space; words by " / ".
//  Returns the number of characters written (excluding NUL).
// ─────────────────────────────────────────────────────────────────────────────
static int textToMorse(const char* text, char* buf, size_t bufSize) {
    if (!buf || bufSize == 0) return 0;
    buf[0] = '\0';
    int written = 0;
    bool firstInWord = true;

    for (size_t i = 0; text[i] != '\0'; ++i) {
        char c = text[i];
        if (c == ' ') {
            if (written + 4 >= (int)bufSize) break;
            strncat(buf, " / ", bufSize - 1 - written);
            written += 3;
            firstInWord = true;
            continue;
        }
        const char* m = charToMorse(c);
        if (!m) continue;
        if (!firstInWord) {
            if (written + 2 >= (int)bufSize) break;
            strncat(buf, " ", bufSize - 1 - written);
            written++;
        }
        int mlen = (int)strlen(m);
        if (written + mlen + 1 >= (int)bufSize) break;
        strncat(buf, m, bufSize - 1 - written);
        written += mlen;
        firstInWord = false;
    }
    return written;
}
