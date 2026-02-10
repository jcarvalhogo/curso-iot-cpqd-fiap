/**
 * @file SecureDeviceAuth.h
 * @brief SecureHttp device-side: builds an authenticated + encrypted HTTP request envelope.
 *
 * This module:
 *  - Encrypts plaintext JSON using AES-256-GCM
 *  - Signs a canonical string using HMAC-SHA256
 *
 * Output is suitable to send via HTTP:
 *  - Body: ciphertextHex
 *  - Headers: X-Device-Id, X-Timestamp, X-Nonce, X-IV, X-Tag, X-Signature
 */

#ifndef SHARED_LIBS_SECUREDEVICEAUTH_H
#define SHARED_LIBS_SECUREDEVICEAUTH_H

#pragma once
#include <Arduino.h>

class SecureDeviceAuth {
public:
    struct Result {
        bool ok = false;
        String error;

        String deviceId;
        String timestamp;      // unix epoch seconds (string decimal)
        String nonce;          // 16 hex chars (8 bytes)
        String ivHex;          // 24 hex chars (12 bytes)
        String tagHex;         // 32 hex chars (16 bytes)
        String signatureHex;   // 64 hex chars (HMAC-SHA256)
        String ciphertextHex;  // body hex
    };

    Result encryptAndSign(const char *deviceId,
                          const char *method,
                          const char *path,
                          const String &plaintextJson) const;

private:
    static String toHex(const uint8_t *data, size_t len);
    static bool fromHex(const String &hex, uint8_t *out, size_t outLen);

    static String randomHex(size_t bytesLen);

    static bool aes256gcmEncrypt(const uint8_t *key32,
                                 const uint8_t *iv12,
                                 const uint8_t *aad, size_t aadLen,
                                 const uint8_t *pt, size_t ptLen,
                                 uint8_t *ct,
                                 uint8_t *tag16);

    static bool hmacSha256Hex(const uint8_t *key, size_t keyLen,
                              const uint8_t *msg, size_t msgLen,
                              String &outHex);

    static String canonicalToSign(const char *method,
                                  const char *path,
                                  const String &deviceId,
                                  const String &timestamp,
                                  const String &nonce,
                                  const String &ivHex,
                                  const String &tagHex,
                                  const String &ciphertextHex);
};

#endif // SHARED_LIBS_SECUREDEVICEAUTH_H
