#pragma once
#include <Arduino.h>

/**
 * @file HexUtils.h
 * @brief HEX encoding/decoding helpers used by SecureHttp.
 */

/**
 * @brief Encode binary data to lowercase hexadecimal string.
 *
 * @param data Pointer to input bytes.
 * @param len Number of bytes to encode.
 * @return Hex string (2*len chars).
 */
String hexEncode(const uint8_t* data, size_t len);

/**
 * @brief Decode a fixed-length hex string into a buffer.
 *
 * This function validates that the input length is exactly @p outLen * 2.
 *
 * @param hexStr Hex input (case-insensitive).
 * @param out Output buffer.
 * @param outLen Expected number of decoded bytes.
 * @return true if decoded successfully; false otherwise.
 */
bool hexDecodeFixed(const String& hexStr, uint8_t* out, size_t outLen);

/**
 * @brief Check if a string is a valid hex string with even length.
 *
 * @param s Input string.
 * @return true if string contains only hex chars and has even length; false otherwise.
 */
bool isHexStringEven(const String& s);
