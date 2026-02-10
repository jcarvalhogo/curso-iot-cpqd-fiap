//
// Created by Josemar Carvalho on 10/02/26.
//

#ifndef GATEWAY_ARDUINO_SECUREHTTPCONFIG_H
#define GATEWAY_ARDUINO_SECUREHTTPCONFIG_H

#pragma once
#include <Arduino.h>

/**
 * @file SecureHttpConfig.h
 * @brief SecureHttp runtime configuration (DO NOT COMMIT).
 *
 * This file contains secrets shared between device and gateway.
 * It must NOT be committed to the repository.
 */

// Identificador do device autorizado
static const char *const SECURE_DEVICE_ID = "vehicle-device-01";

// Segredo do HMAC (use algo grande e aleatório)
static const char *const SECURE_HMAC_SECRET =
        "CHANGE_ME_USE_A_LONG_RANDOM_SECRET_32_PLUS_CHARS";

// Chave AES-256 (32 bytes EXATOS)
// Gere uma chave aleatória real para produção
static const uint8_t SECURE_AES_KEY[32] = {
    0x10, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x01,
    0x02, 0x13, 0x24, 0x35, 0x46, 0x57, 0x68, 0x79,
    0x8a, 0x9b, 0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1
};

// Janela de tempo aceita (anti-replay) em segundos
static const uint32_t SECURE_TS_WINDOW_SEC = 180;

// Cache de nonce (anti-replay)
static const size_t SECURE_NONCE_CACHE_CAP = 48;
static const uint32_t SECURE_NONCE_TTL_SEC = 300;


#endif //GATEWAY_ARDUINO_SECUREHTTPCONFIG_H
