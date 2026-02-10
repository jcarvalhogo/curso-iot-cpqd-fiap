#pragma once
#include <Arduino.h>

/**
 * @file SecureHttpConfig.example.h
 * @brief Example configuration for SecureHttp.
 *
 * Copy this file to SecureHttpConfig.h and fill values.
 * Keep SecureHttpConfig.h out of version control.
 */

static const char* const SECURE_DEVICE_ID = "vehicle-001";

static const char* const SECURE_HMAC_SECRET =
  "CHANGE_ME_USE_A_LONG_RANDOM_SECRET_32+";

static const uint8_t SECURE_AES_KEY[32] = {
  0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
  0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
  0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
  0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f
};

static const uint32_t SECURE_TS_WINDOW_SEC = 180;

static const size_t   SECURE_NONCE_CACHE_CAP = 48;
static const uint32_t SECURE_NONCE_TTL_SEC = 300;
