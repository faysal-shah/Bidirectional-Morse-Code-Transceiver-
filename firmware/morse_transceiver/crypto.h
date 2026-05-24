/**
 * @file    crypto.h
 * @brief   AES-128-CBC encryption + HMAC-SHA256 authentication (Encrypt-then-MAC).
 *
 * Uses mbedTLS, which ships with the ESP32 Arduino core — no extra library needed.
 *
 * Packet layout written by cryptoEncrypt():
 *
 *   ┌──────────────┬───────────────────┬───────────────────────┐
 *   │  IV (16 B)   │  HMAC-SHA256 (32B)│  AES-CBC ciphertext   │
 *   └──────────────┴───────────────────┴───────────────────────┘
 *
 * The HMAC covers IV || ciphertext, preventing any tampering with either field.
 * PKCS#7 padding is applied before encryption and stripped after decryption.
 */
#pragma once
#include <Arduino.h>
#include "mbedtls/aes.h"
#include "mbedtls/md.h"
#include "config.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Constants
// ─────────────────────────────────────────────────────────────────────────────
#define CRYPTO_IV_LEN    16
#define CRYPTO_HMAC_LEN  32
#define CRYPTO_BLOCK_LEN 16
#define CRYPTO_OVERHEAD  (CRYPTO_IV_LEN + CRYPTO_HMAC_LEN)  // 48 bytes

// Returns the encrypted buffer size needed for a plaintext of `ptLen` bytes.
static inline size_t cryptoEncSize(size_t ptLen) {
    // Round up to next AES block, then add header overhead
    return CRYPTO_OVERHEAD + ((ptLen / CRYPTO_BLOCK_LEN) + 1) * CRYPTO_BLOCK_LEN;
}

// ─────────────────────────────────────────────────────────────────────────────
//  cryptoEncrypt
//  Encrypts `ptLen` bytes of `plaintext` into `out`.
//  `out` must hold at least cryptoEncSize(ptLen) bytes.
//  Returns bytes written, or 0 on error.
// ─────────────────────────────────────────────────────────────────────────────
static size_t cryptoEncrypt(const uint8_t* plaintext, size_t ptLen,
                             uint8_t* out,             size_t outSize) {
    if (!plaintext || !out || ptLen == 0) return 0;
    if (outSize < cryptoEncSize(ptLen))   return 0;

    uint8_t* iv_field   = out;
    uint8_t* hmac_field = out + CRYPTO_IV_LEN;
    uint8_t* ct_field   = out + CRYPTO_OVERHEAD;

    // 1. Random IV using ESP32 hardware TRNG
    esp_fill_random(iv_field, CRYPTO_IV_LEN);

    // 2. PKCS#7 padding
    size_t padded = ((ptLen / CRYPTO_BLOCK_LEN) + 1) * CRYPTO_BLOCK_LEN;
    uint8_t padBuf[padded];
    memcpy(padBuf, plaintext, ptLen);
    uint8_t pad = (uint8_t)(padded - ptLen);
    memset(padBuf + ptLen, pad, pad);

    // 3. AES-128-CBC encrypt  (modifies iv_copy, so use a copy)
    uint8_t iv_copy[CRYPTO_IV_LEN];
    memcpy(iv_copy, iv_field, CRYPTO_IV_LEN);

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, (const uint8_t*)AES_KEY, 128);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT,
                          padded, iv_copy, padBuf, ct_field);
    mbedtls_aes_free(&aes);

    // 4. HMAC-SHA256 over  IV || ciphertext  (Encrypt-then-MAC)
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
    mbedtls_md_hmac_starts(&ctx, (const uint8_t*)HMAC_KEY, strlen(HMAC_KEY));
    mbedtls_md_hmac_update(&ctx, iv_field, CRYPTO_IV_LEN);
    mbedtls_md_hmac_update(&ctx, ct_field, padded);
    mbedtls_md_hmac_finish(&ctx, hmac_field);
    mbedtls_md_free(&ctx);

    return CRYPTO_OVERHEAD + padded;
}

// ─────────────────────────────────────────────────────────────────────────────
//  cryptoDecrypt
//  Verifies HMAC, then decrypts `encLen` bytes of `encrypted` into `out`.
//  `out` must hold at least (encLen - CRYPTO_OVERHEAD) bytes.
//  Returns plaintext byte count (NUL-terminated), or 0 on auth/padding failure.
// ─────────────────────────────────────────────────────────────────────────────
static size_t cryptoDecrypt(const uint8_t* encrypted, size_t encLen,
                             uint8_t*       out,        size_t outSize) {
    if (!encrypted || !out || encLen <= CRYPTO_OVERHEAD) return 0;

    const uint8_t* iv_field   = encrypted;
    const uint8_t* hmac_field = encrypted + CRYPTO_IV_LEN;
    const uint8_t* ct_field   = encrypted + CRYPTO_OVERHEAD;
    size_t ctLen = encLen - CRYPTO_OVERHEAD;

    if (ctLen % CRYPTO_BLOCK_LEN != 0) return 0;
    if (outSize < ctLen)               return 0;

    // 1. Recompute HMAC and compare (constant-time to defeat timing attacks)
    uint8_t computed[CRYPTO_HMAC_LEN];
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
    mbedtls_md_hmac_starts(&ctx, (const uint8_t*)HMAC_KEY, strlen(HMAC_KEY));
    mbedtls_md_hmac_update(&ctx, iv_field, CRYPTO_IV_LEN);
    mbedtls_md_hmac_update(&ctx, ct_field, ctLen);
    mbedtls_md_hmac_finish(&ctx, computed);
    mbedtls_md_free(&ctx);

    uint8_t diff = 0;
    for (int i = 0; i < CRYPTO_HMAC_LEN; ++i) diff |= (hmac_field[i] ^ computed[i]);
    if (diff != 0) {
        Serial.println("[CRYPTO] ✗ HMAC mismatch — packet rejected");
        return 0;
    }

    // 2. AES-128-CBC decrypt
    uint8_t iv_copy[CRYPTO_IV_LEN];
    memcpy(iv_copy, iv_field, CRYPTO_IV_LEN);

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_dec(&aes, (const uint8_t*)AES_KEY, 128);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT,
                          ctLen, iv_copy, ct_field, out);
    mbedtls_aes_free(&aes);

    // 3. Strip PKCS#7 padding
    uint8_t padByte = out[ctLen - 1];
    if (padByte == 0 || padByte > CRYPTO_BLOCK_LEN) return 0;
    for (size_t i = ctLen - padByte; i < ctLen; ++i) {
        if (out[i] != padByte) return 0;   // Corrupted padding
    }
    size_t ptLen = ctLen - padByte;
    out[ptLen] = '\0';  // Convenient NUL terminator
    return ptLen;
}
