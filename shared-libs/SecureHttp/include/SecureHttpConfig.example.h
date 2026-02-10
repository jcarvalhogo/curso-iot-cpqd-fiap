#ifndef SHARED_LIBS_SECUREHTTP_SECUREHTTPCONFIG_H
#define SHARED_LIBS_SECUREHTTP_SECUREHTTPCONFIG_H

#pragma once
#include <Arduino.h>

// Authorized device id
static const char *const SECURE_DEVICE_ID = "vehicle-device-01";

// HMAC secret as literal (ASCII). Keep >= 32 chars.
#define SECURE_HMAC_SECRET "CHANGE_ME_USE_A_LONG_RANDOM_SECRET_32_PLUS_CHARS"

// AES-256 key (32 bytes)
static const uint8_t SECURE_AES_KEY[32] = {
  0x10,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
  0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff,0x01,
  0x02,0x13,0x24,0x35,0x46,0x57,0x68,0x79,
  0x8a,0x9b,0xac,0xbd,0xce,0xdf,0xe0,0xf1
};

// Timestamp window (seconds)
static const uint32_t SECURE_TS_WINDOW_SEC = 180;

// Nonce cache (anti-replay)
static const size_t SECURE_NONCE_CACHE_CAP = 48;
static const uint32_t SECURE_NONCE_TTL_SEC = 300;

// Compatibility symbols expected by SecureDeviceAuth / SecureGatewayAuth
#define SECUREHTTP_AES256_KEY SECURE_AES_KEY
static const uint8_t* const SECUREHTTP_HMAC_KEY =
  reinterpret_cast<const uint8_t*>(SECURE_HMAC_SECRET);
static const size_t SECUREHTTP_HMAC_KEY_LEN = sizeof(SECURE_HMAC_SECRET) - 1;

#endif // SHARED_LIBS_SECUREHTTP_SECUREHTTPCONFIG_H
