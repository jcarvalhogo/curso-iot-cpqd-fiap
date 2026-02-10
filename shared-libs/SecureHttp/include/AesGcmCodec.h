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

/**
 * @brief AES-GCM encryption output.
 */
struct AesGcmEncrypted {
  String cipherHex; ///< Ciphertext hex (2 * plaintext length chars).
  String tagHex;    ///< Authentication tag hex (32 chars).
};

/**
 * @brief AES-256-GCM codec utilities.
 */
class AesGcmCodec {
public:
  /**
   * @brief Encrypt a plaintext string using AES-256-GCM.
   *
   * @param key AES key (32 bytes).
   * @param keyLen Must be 32.
   * @param iv IV/nonce (12 bytes).
   * @param ivLen Must be 12.
   * @param plaintext Data to encrypt.
   * @return AesGcmEncrypted containing hex ciphertext and hex tag. Empty fields indicate failure.
   */
  static AesGcmEncrypted encrypt(
      const uint8_t* key, size_t keyLen,
      const uint8_t* iv, size_t ivLen,
      const String& plaintext);

  /**
   * @brief Decrypt ciphertext using AES-256-GCM.
   *
   * @param key AES key (32 bytes).
   * @param keyLen Must be 32.
   * @param iv IV/nonce (12 bytes).
   * @param ivLen Must be 12.
   * @param cipher Ciphertext bytes.
   * @param cipherLen Ciphertext length.
   * @param tag Authentication tag (16 bytes).
   * @param tagLen Must be 16.
   * @return Decrypted plaintext string, or empty string on failure (including auth failure).
   */
  static String decrypt(
      const uint8_t* key, size_t keyLen,
      const uint8_t* iv, size_t ivLen,
      const uint8_t* cipher, size_t cipherLen,
      const uint8_t* tag, size_t tagLen);
};
