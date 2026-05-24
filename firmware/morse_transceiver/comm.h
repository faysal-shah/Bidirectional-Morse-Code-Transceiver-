/**
 * @file    comm.h
 * @brief   ESP-NOW communication layer — encrypted peer-to-peer messaging.
 *
 * Architecture
 * ────────────
 *  • Uses Wi-Fi in STATION mode (no router needed) with ESP-NOW on top.
 *  • Default peer address is broadcast (FF:FF:FF:FF:FF:FF); replace with the
 *    specific peer MAC in config.h after you discover it via the Serial log.
 *  • Every payload is AES-128-CBC + HMAC-SHA256 encrypted (see crypto.h).
 *  • Received frames are pushed to a small ring buffer processed in loop().
 *
 * Packet structure  (max 250 bytes — ESP-NOW limit)
 * ──────────────────────────────────────────────────
 *   ┌──────────────────────────┬────────────────────────────────────────────┐
 *   │  MLHeader  (8 bytes)     │  Encrypted payload  (up to 202 bytes)      │
 *   └──────────────────────────┴────────────────────────────────────────────┘
 */
#pragma once
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "config.h"
#include "crypto.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Protocol header  (packed to avoid struct-padding surprises)
// ─────────────────────────────────────────────────────────────────────────────
#define ESPNOW_MAX_PAYLOAD  250

struct __attribute__((packed)) MLHeader {
    uint16_t magic;      // PROTO_MAGIC  ('ML')
    uint8_t  version;    // PROTO_VERSION
    uint8_t  type;       // Packet type (see PKT_TYPE_*)
    uint8_t  src_id;     // Sending device ID
    uint8_t  dst_id;     // Target device ID (0xFF = broadcast)
    uint16_t seq;        // Rolling sequence number (uint16 wraps at 65535)
    uint8_t  data_len;   // Encrypted payload length
};

struct __attribute__((packed)) MLPacket {
    MLHeader hdr;
    uint8_t  payload[ESPNOW_MAX_PAYLOAD - sizeof(MLHeader)];
};

// Packet types
#define PKT_TYPE_MSG   0x01   // Text message
#define PKT_TYPE_ACK   0x02   // Delivery acknowledgement
#define PKT_TYPE_PING  0x03   // Keep-alive (future use)

// ─────────────────────────────────────────────────────────────────────────────
//  Receive ring buffer  (written in ISR, read in loop())
// ─────────────────────────────────────────────────────────────────────────────
#define RX_QUEUE_DEPTH  4

struct RxEntry {
    uint8_t srcMac[6];
    uint8_t data[ESPNOW_MAX_PAYLOAD];
    uint8_t len;
    bool    ready;
};

static volatile RxEntry rxQueue[RX_QUEUE_DEPTH];
static volatile int     rxHead = 0;   // Written by ISR
static volatile int     rxTail = 0;   // Read by loop()

// ─────────────────────────────────────────────────────────────────────────────
//  Module state
// ─────────────────────────────────────────────────────────────────────────────
static bool     commReady  = false;
static uint16_t seqCounter = 0;
static uint8_t  peerMac[6] = PEER_MAC_ADDR;
static bool     lastSendOK = false;

// ─────────────────────────────────────────────────────────────────────────────
//  ESP-NOW callbacks
// ─────────────────────────────────────────────────────────────────────────────
static void IRAM_ATTR onDataReceived(const esp_now_recv_info_t* info,
                                     const uint8_t* data, int len) {
    int next = (rxHead + 1) % RX_QUEUE_DEPTH;
    if (next == rxTail) return;   // Ring buffer full — drop frame

    RxEntry* e = (RxEntry*)&rxQueue[rxHead];
    memcpy(e->srcMac, info->src_addr, 6);
    int copyLen = (len < (int)sizeof(e->data)) ? len : (int)sizeof(e->data);
    memcpy(e->data, data, copyLen);
    e->len   = (uint8_t)copyLen;
    e->ready = true;
    rxHead   = next;
}

static void onDataSent(const uint8_t* /*mac*/, esp_now_send_status_t status) {
    lastSendOK = (status == ESP_NOW_SEND_SUCCESS);
    if (lastSendOK) digitalWrite(PIN_LED, LOW);
}

// ─────────────────────────────────────────────────────────────────────────────
//  commInit  — call once in setup()
// ─────────────────────────────────────────────────────────────────────────────
static bool commInit() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(false);

    uint8_t myMac[6];
    WiFi.macAddress(myMac);
    Serial.printf("[COMM] MAC → %02X:%02X:%02X:%02X:%02X:%02X\n",
                  myMac[0], myMac[1], myMac[2],
                  myMac[3], myMac[4], myMac[5]);

    if (esp_now_init() != ESP_OK) {
        Serial.println("[COMM] ✗ esp_now_init failed");
        return false;
    }

    esp_now_register_recv_cb(onDataReceived);
    esp_now_register_send_cb(onDataSent);

    // Register peer
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, peerMac, 6);
    peer.channel = ESPNOW_CHANNEL;
    peer.encrypt = false;   // Handled by our crypto layer

    if (!esp_now_is_peer_exist(peerMac)) {
        if (esp_now_add_peer(&peer) != ESP_OK) {
            Serial.println("[COMM] ✗ esp_now_add_peer failed");
            return false;
        }
    }

    Serial.printf("[COMM] ✓ ESP-NOW ready (channel %d)\n", ESPNOW_CHANNEL);
    commReady = true;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  commSendMessage  — encrypt and transmit a text string
//  Returns true if esp_now_send() accepted the frame (not a delivery receipt).
// ─────────────────────────────────────────────────────────────────────────────
static bool commSendMessage(const char* text, uint8_t dstId = 0xFF) {
    if (!commReady || !text) return false;
    size_t textLen = strlen(text);
    if (textLen == 0 || textLen >= MAX_MSG_CHARS) return false;

    // Encrypt
    MLPacket pkt = {};
    size_t maxPayload = sizeof(pkt.payload);
    size_t encLen = cryptoEncrypt((const uint8_t*)text, textLen,
                                  pkt.payload, maxPayload);
    if (encLen == 0) {
        Serial.println("[COMM] ✗ Encryption failed");
        return false;
    }

    // Fill header
    pkt.hdr.magic    = PROTO_MAGIC;
    pkt.hdr.version  = PROTO_VERSION;
    pkt.hdr.type     = PKT_TYPE_MSG;
    pkt.hdr.src_id   = DEVICE_ID;
    pkt.hdr.dst_id   = dstId;
    pkt.hdr.seq      = seqCounter++;
    pkt.hdr.data_len = (uint8_t)encLen;

    digitalWrite(PIN_LED, HIGH);
    esp_err_t err = esp_now_send(peerMac, (uint8_t*)&pkt,
                                 sizeof(MLHeader) + encLen);

    if (err != ESP_OK) {
        Serial.printf("[COMM] ✗ esp_now_send error: %d\n", err);
        digitalWrite(PIN_LED, LOW);
        return false;
    }

    Serial.printf("[COMM] ↑ Sent %zu bytes (seq=%u)\n", encLen, pkt.hdr.seq);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  commReceive  — call every loop() iteration
//  Writes decoded plaintext into `outText` (size `outSize`).
//  Returns true when a valid message frame was dequeued.
// ─────────────────────────────────────────────────────────────────────────────
static bool commReceive(char* outText, size_t outSize, uint8_t* srcId = nullptr) {
    if (!outText || rxTail == rxHead) return false;   // Nothing queued

    RxEntry* e = (RxEntry*)&rxQueue[rxTail];
    if (!e->ready) {
        rxTail = (rxTail + 1) % RX_QUEUE_DEPTH;
        return false;
    }

    bool result = false;

    do {
        if (e->len < (int)sizeof(MLHeader)) break;

        const MLPacket* pkt = (const MLPacket*)e->data;

        // Basic header validation
        if (pkt->hdr.magic   != PROTO_MAGIC)   break;
        if (pkt->hdr.version != PROTO_VERSION) break;

        // Addressing filter — accept broadcast or messages to us
        if (pkt->hdr.dst_id != 0xFF && pkt->hdr.dst_id != DEVICE_ID) break;

        // Ignore our own reflections (can happen with broadcast)
        if (pkt->hdr.src_id == DEVICE_ID) break;

        if (pkt->hdr.type != PKT_TYPE_MSG) break;

        // Decrypt
        uint8_t plain[MAX_MSG_CHARS + 1];
        size_t ptLen = cryptoDecrypt(pkt->payload, pkt->hdr.data_len,
                                     plain, sizeof(plain));
        if (ptLen == 0) break;

        size_t copyLen = (ptLen < outSize - 1) ? ptLen : outSize - 1;
        memcpy(outText, plain, copyLen);
        outText[copyLen] = '\0';
        if (srcId) *srcId = pkt->hdr.src_id;

        Serial.printf("[COMM] ↓ Rx from device %u: \"%s\"\n",
                      pkt->hdr.src_id, outText);
        result = true;
    } while (false);

    e->ready = false;
    rxTail   = (rxTail + 1) % RX_QUEUE_DEPTH;
    return result;
}
