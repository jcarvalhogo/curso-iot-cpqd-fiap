#include "AesGcmCodec.h"
#include "HexUtils.h"
#include "mbedtls/gcm.h"
#include <memory>

AesGcmEncrypted AesGcmCodec::encrypt(
    const uint8_t* key, size_t keyLen,
    const uint8_t* iv, size_t ivLen,
    const String& plaintext) {

  AesGcmEncrypted out;
  if (keyLen != 32 || ivLen != 12) return out;

  const size_t n = plaintext.length();
  if (n == 0) { out.cipherHex = ""; out.tagHex = ""; return out; }

  std::unique_ptr<uint8_t[]> cipher(new uint8_t[n]);
  uint8_t tag[16];

  mbedtls_gcm_context ctx;
  mbedtls_gcm_init(&ctx);

  if (mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, key, (unsigned int)(keyLen * 8)) != 0) {
    mbedtls_gcm_free(&ctx);
    return out;
  }

  int ret = mbedtls_gcm_crypt_and_tag(
      &ctx,
      MBEDTLS_GCM_ENCRYPT,
      n,
      iv, ivLen,
      nullptr, 0, // AAD
      (const uint8_t*)plaintext.c_str(),
      cipher.get(),
      sizeof(tag),
      tag);

  mbedtls_gcm_free(&ctx);
  if (ret != 0) return out;

  out.cipherHex = hexEncode(cipher.get(), n);
  out.tagHex = hexEncode(tag, sizeof(tag));
  return out;
}

String AesGcmCodec::decrypt(
    const uint8_t* key, size_t keyLen,
    const uint8_t* iv, size_t ivLen,
    const uint8_t* cipher, size_t cipherLen,
    const uint8_t* tag, size_t tagLen) {

  if (keyLen != 32 || ivLen != 12 || tagLen != 16) return "";

  std::unique_ptr<uint8_t[]> plain(new uint8_t[cipherLen + 1]);
  plain.get()[cipherLen] = 0;

  mbedtls_gcm_context ctx;
  mbedtls_gcm_init(&ctx);

  if (mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, key, 256) != 0) {
    mbedtls_gcm_free(&ctx);
    return "";
  }

  int ret = mbedtls_gcm_auth_decrypt(
      &ctx, cipherLen,
      iv, ivLen,
      nullptr, 0,
      tag, tagLen,
      cipher,
      plain.get());

  mbedtls_gcm_free(&ctx);
  if (ret != 0) return "";

  return String((const char*)plain.get()).substring(0, cipherLen);
}
