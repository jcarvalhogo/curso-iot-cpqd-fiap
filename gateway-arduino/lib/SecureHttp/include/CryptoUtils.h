#pragma once
#include <Arduino.h>

/**
 * @file CryptoUtils.h
 * @brief SHA-256 and HMAC-SHA256 helpers for SecureHttp.
 */

/**
 * @brief Compute SHA-256 over a string and return the digest as lowercase hex.
 *
 * @param s Input string.
 * @return 64-char hex digest.
 */
String sha256Hex(const String& s);

/**
 * @brief Compute HMAC-SHA256(key, msg) and return digest as lowercase hex.
 *
 * @param key Shared secret.
 * @param msg Message to authenticate.
 * @return 64-char hex digest or empty string if the underlying crypto setup failed.
 */
String hmacSha256Hex(const String& key, const String& msg);

/**
 * @brief Constant-time string equality check.
 *
 * Prevents timing side-channel leakage that could occur with early-exit comparisons.
 *
 * @param a First string.
 * @param b Second string.
 * @return true if equal; false otherwise.
 */
bool constantTimeEquals(const String& a, const String& b);
