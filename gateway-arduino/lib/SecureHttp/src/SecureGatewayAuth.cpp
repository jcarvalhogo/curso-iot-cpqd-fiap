#include "SecureGatewayAuth.h"
#include "HexUtils.h"
#include "CryptoUtils.h"
#include "AesGcmCodec.h"

#include <memory>

SecureGatewayAuth::SecureGatewayAuth()
  : _nonceCache(SECURE_NONCE_CACHE_CAP, SECURE_NONCE_TTL_SEC) {}

uint32_t SecureGatewayAuth::nowSec() {
  return (uint32_t)(millis() / 1000);
}

static bool decodeHexToBuf(const String& hexStr, std::unique_ptr<uint8_t[]>& bufOut, size_t& lenOut) {
  if (hexStr.isEmpty() || !isHexStringEven(hexStr)) return false;
  lenOut = (size_t)hexStr.length() / 2;
  bufOut.reset(new uint8_t[lenOut]);
  for (size_t i = 0; i < lenOut; i++) {
    unsigned int v = 0;
    if (sscanf(hexStr.substring(i * 2, i * 2 + 2).c_str(), "%02x", &v) != 1) return false;
    bufOut.get()[i] = (uint8_t)v;
  }
  return true;
}

SecureAuthResult SecureGatewayAuth::verifyAndDecrypt(WebServer& server, const String& method, const String& path) {
  SecureAuthResult r;

  const String deviceId  = server.header("X-Device-Id");
  const String tsStr     = server.header("X-Timestamp");
  const String nonce     = server.header("X-Nonce");
  const String ivHex     = server.header("X-IV");
  const String tagHex    = server.header("X-Tag");
  const String signature = server.header("X-Signature");

  if (deviceId.isEmpty() || tsStr.isEmpty() || nonce.isEmpty() || ivHex.isEmpty() ||
      tagHex.isEmpty() || signature.isEmpty()) {
    r.httpCode = 400;
    r.error = "missing_headers";
    return r;
  }

  if (String(SECURE_DEVICE_ID) != deviceId) {
    r.httpCode = 401;
    r.error = "unknown_device";
    return r;
  }

  const uint32_t ts = (uint32_t)tsStr.toInt();
  const uint32_t now = nowSec();
  if (ts == 0) {
    r.httpCode = 401;
    r.error = "bad_timestamp";
    return r;
  }

  const uint32_t diff = (now > ts) ? (now - ts) : (ts - now);
  if (diff > SECURE_TS_WINDOW_SEC) {
    r.httpCode = 401;
    r.error = "timestamp_out_of_window";
    return r;
  }

  if (_nonceCache.seenRecently(now, nonce)) {
    r.httpCode = 401;
    r.error = "replay_nonce";
    return r;
  }

  const String bodyCipherHex = server.arg("plain");
  if (bodyCipherHex.isEmpty() || !isHexStringEven(bodyCipherHex)) {
    r.httpCode = 400;
    r.error = "bad_body";
    return r;
  }

  const String bodyHash = sha256Hex(bodyCipherHex);
  const String canonical = deviceId + "\n" + tsStr + "\n" + nonce + "\n" + method + "\n" + path + "\n" + bodyHash;
  const String expected = hmacSha256Hex(String(SECURE_HMAC_SECRET), canonical);

  if (expected.isEmpty() || !constantTimeEquals(expected, signature)) {
    r.httpCode = 401;
    r.error = "bad_signature";
    return r;
  }

  uint8_t iv[12];
  uint8_t tag[16];
  if (!hexDecodeFixed(ivHex, iv, sizeof(iv)) || !hexDecodeFixed(tagHex, tag, sizeof(tag))) {
    r.httpCode = 400;
    r.error = "bad_iv_or_tag";
    return r;
  }

  std::unique_ptr<uint8_t[]> cipher;
  size_t cipherLen = 0;
  if (!decodeHexToBuf(bodyCipherHex, cipher, cipherLen)) {
    r.httpCode = 400;
    r.error = "bad_cipher_hex";
    return r;
  }

  String plain = AesGcmCodec::decrypt(SECURE_AES_KEY, sizeof(SECURE_AES_KEY), iv, sizeof(iv), cipher.get(), cipherLen, tag, sizeof(tag));
  if (plain.isEmpty()) {
    r.httpCode = 401;
    r.error = "decrypt_failed";
    return r;
  }

  _nonceCache.remember(now, nonce);

  r.ok = true;
  r.httpCode = 200;
  r.plaintextJson = plain;
  return r;
}
