#include "SecureGatewayAuth.h"

#include "HexUtils.h"
#include "CryptoUtils.h"
#include "AesGcmCodec.h"

#include <memory>
#include <time.h>

#include <mbedtls/md.h>   // HMAC-SHA256

SecureGatewayAuth::SecureGatewayAuth()
  : _nonceCache(SECURE_NONCE_CACHE_CAP, SECURE_NONCE_TTL_SEC) {}

uint32_t SecureGatewayAuth::nowSec() {
  time_t now = time(nullptr);
  if (now > 1700000000) return (uint32_t)now;   // epoch OK
  return (uint32_t)(millis() / 1000);          // fallback
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

static String toHexLocal(const uint8_t* data, size_t len) {
  static const char* hex = "0123456789abcdef";
  String out;
  out.reserve(len * 2);
  for (size_t i = 0; i < len; i++) {
    out += hex[(data[i] >> 4) & 0x0F];
    out += hex[data[i] & 0x0F];
  }
  return out;
}

static bool hmacSha256HexBytes(const uint8_t* key, size_t keyLen,
                              const uint8_t* msg, size_t msgLen,
                              String& outHex) {
  const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (!info) return false;

  uint8_t mac[32];
  int rc = mbedtls_md_hmac(info, key, keyLen, msg, msgLen, mac);
  if (rc != 0) return false;

  outHex = toHexLocal(mac, sizeof(mac));
  return true;
}

static String canonicalToSign(const String& method,
                              const String& path,
                              const String& deviceId,
                              const String& timestamp,
                              const String& nonce,
                              const String& ivHex,
                              const String& tagHex,
                              const String& ciphertextHex) {
  // MUST match device:
  // METHOD\nPATH\nDEVICE\nTS\nNONCE\nIV\nTAG\nCIPHERTEXT
  String s;
  s.reserve(64 + ciphertextHex.length());
  s += method; s += "\n";
  s += path; s += "\n";
  s += deviceId; s += "\n";
  s += timestamp; s += "\n";
  s += nonce; s += "\n";
  s += ivHex; s += "\n";
  s += tagHex; s += "\n";
  s += ciphertextHex;
  return s;
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

  const String bodyCipherHex = server.arg("plain"); // body raw (hex)
  if (bodyCipherHex.isEmpty() || !isHexStringEven(bodyCipherHex)) {
    r.httpCode = 400;
    r.error = "bad_body";
    return r;
  }

  // HMAC verify (same canonical as device)
  const String canonical = canonicalToSign(method, path, deviceId, tsStr, nonce, ivHex, tagHex, bodyCipherHex);

  String expected;
  if (!hmacSha256HexBytes(SECUREHTTP_HMAC_KEY, SECUREHTTP_HMAC_KEY_LEN,
                          (const uint8_t*)canonical.c_str(), canonical.length(),
                          expected)) {
    r.httpCode = 401;
    r.error = "hmac_failed";
    return r;
  }

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

  // AAD MUST match device
  String aadStr;
  aadStr.reserve(64);
  aadStr += deviceId; aadStr += "|";
  aadStr += tsStr;    aadStr += "|";
  aadStr += nonce;    aadStr += "|";
  aadStr += method;   aadStr += "|";
  aadStr += path;

  // Decrypt AES-256-GCM WITH AAD
  String plain = AesGcmCodec::decryptWithAad(
      SECUREHTTP_AES256_KEY, sizeof(SECUREHTTP_AES256_KEY),
      iv, sizeof(iv),
      (const uint8_t*)aadStr.c_str(), aadStr.length(),
      cipher.get(), cipherLen,
      tag, sizeof(tag)
  );

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