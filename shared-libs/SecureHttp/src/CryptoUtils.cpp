#include "CryptoUtils.h"
#include "mbedtls/md.h"
#include "mbedtls/sha256.h"

String sha256Hex(const String& s) {
  uint8_t hash[32];
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts_ret(&ctx, 0);
  mbedtls_sha256_update_ret(&ctx, (const unsigned char*)s.c_str(), s.length());
  mbedtls_sha256_finish_ret(&ctx, hash);
  mbedtls_sha256_free(&ctx);

  static const char* hex = "0123456789abcdef";
  String out;
  out.reserve(sizeof(hash) * 2);
  for (size_t i = 0; i < sizeof(hash); i++) {
    out += hex[(hash[i] >> 4) & 0xF];
    out += hex[hash[i] & 0xF];
  }
  return out;
}

String hmacSha256Hex(const String& key, const String& msg) {
  uint8_t hmac[32];
  const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  if (mbedtls_md_setup(&ctx, info, 1) != 0) {
    mbedtls_md_free(&ctx);
    return "";
  }

  mbedtls_md_hmac_starts(&ctx, (const unsigned char*)key.c_str(), key.length());
  mbedtls_md_hmac_update(&ctx, (const unsigned char*)msg.c_str(), msg.length());
  mbedtls_md_hmac_finish(&ctx, hmac);
  mbedtls_md_free(&ctx);

  static const char* hex = "0123456789abcdef";
  String out;
  out.reserve(sizeof(hmac) * 2);
  for (size_t i = 0; i < sizeof(hmac); i++) {
    out += hex[(hmac[i] >> 4) & 0xF];
    out += hex[hmac[i] & 0xF];
  }
  return out;
}

bool constantTimeEquals(const String& a, const String& b) {
  if (a.length() != b.length()) return false;
  uint8_t diff = 0;
  for (size_t i = 0; i < (size_t)a.length(); i++) diff |= (uint8_t)(a[i] ^ b[i]);
  return diff == 0;
}
