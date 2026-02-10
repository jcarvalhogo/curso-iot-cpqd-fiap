//
// Created by Josemar Carvalho on 10/02/26.
//

#ifndef SHARED_LIBS_AESGCMCODEC_H
#define SHARED_LIBS_AESGCMCODEC_H

#pragma once
#include <Arduino.h>

/**
 * @file AesGcmCodec.h
 * @brief AES-256-GCM encrypt/decrypt helper for SecureHttp.
 *
 * AES-GCM provides confidentiality and integrity of the ciphertext via an auth tag.
 * This library uses:
 * - Key: 32 bytes (AES-256)
 * - IV:  12 bytes (recommended length for GCM)
 * - Tag: 16 bytes
 */

struct AesGcmEncrypted {
    String cipherHex; ///< Ciphertext hex (2 * plaintext length chars).
    String tagHex;    ///< Authentication tag hex (32 chars).
};

class AesGcmCodec {
public:
    // === Sem AAD (mantido para compatibilidade) ===
    static AesGcmEncrypted encrypt(
        const uint8_t* key, size_t keyLen,
        const uint8_t* iv, size_t ivLen,
        const String& plaintext);

    static String decrypt(
        const uint8_t* key, size_t keyLen,
        const uint8_t* iv, size_t ivLen,
        const uint8_t* cipher, size_t cipherLen,
        const uint8_t* tag, size_t tagLen);

    // === Com AAD (necessário para o protocolo atual device/gateway) ===
    static AesGcmEncrypted encryptWithAad(
        const uint8_t* key, size_t keyLen,
        const uint8_t* iv, size_t ivLen,
        const uint8_t* aad, size_t aadLen,
        const String& plaintext);

    static String decryptWithAad(
        const uint8_t* key, size_t keyLen,
        const uint8_t* iv, size_t ivLen,
        const uint8_t* aad, size_t aadLen,
        const uint8_t* cipher, size_t cipherLen,
        const uint8_t* tag, size_t tagLen);
};

#endif //SHARED_LIBS_AESGCMCODEC_H