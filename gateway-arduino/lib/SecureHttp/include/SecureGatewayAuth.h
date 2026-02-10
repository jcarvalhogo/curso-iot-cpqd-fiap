#pragma once
#include <Arduino.h>
#include <WebServer.h>

#include "SecureHttpConfig.h" // user-provided (copy from .example)
#include "NonceCache.h"

/**
 * @file SecureGatewayAuth.h
 * @brief Gateway-side verification and decryption for SecureHttp requests.
 *
 * The gateway expects:
 * - HTTP body: ciphertext hex string (not JSON)
 * - Headers:
 *   - X-Device-Id
 *   - X-Timestamp (uint seconds)
 *   - X-Nonce
 *   - X-IV (12 bytes hex)
 *   - X-Tag (16 bytes hex)
 *   - X-Signature (HMAC-SHA256 hex)
 *
 * HMAC canonical string:
 *   deviceId + "\n" + timestamp + "\n" + nonce + "\n" + method + "\n" + path + "\n" + sha256Hex(bodyCipherHex)
 */

/**
 * @brief Result object returned by SecureGatewayAuth.
 */
struct SecureAuthResult {
  bool ok = false;        ///< true on success.
  int httpCode = 401;     ///< HTTP status code suggested for response.
  String error;           ///< Error code string (stable identifiers).
  String plaintextJson;   ///< Decrypted JSON payload (only valid if ok==true).
};

/**
 * @brief Request verifier and AES-GCM decryptor for the gateway.
 */
class SecureGatewayAuth {
public:
  /**
   * @brief Construct a verifier/decryptor using configured cache parameters.
   */
  SecureGatewayAuth();

  /**
   * @brief Get "now" in seconds used for timestamp checks.
   *
   * Default implementation uses millis()/1000.
   * If you have real time (SNTP), you can replace this to use time(nullptr).
   *
   * @return Current time in seconds.
   */
  static uint32_t nowSec();

  /**
   * @brief Verify headers + anti-replay + HMAC signature, then AES-GCM decrypt.
   *
   * @param server WebServer instance handling the request.
   * @param method HTTP method used by the client (e.g. "POST").
   * @param path HTTP path used by the client (e.g. "/telemetry"). Must match what the client signed.
   * @return SecureAuthResult with decrypted JSON on success.
   */
  SecureAuthResult verifyAndDecrypt(WebServer& server, const String& method, const String& path);

private:
  NonceCache _nonceCache;
};
