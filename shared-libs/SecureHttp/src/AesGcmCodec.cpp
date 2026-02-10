#include "AesGcmCodec.h"
#include "HexUtils.h"
#include "mbedtls/gcm.h"
#include <memory>

#include "AesGcmCodec.h"
#include "HexUtils.h"
#include "mbedtls/gcm.h"
#include <memory>

static bool setKey(mbedtls_gcm_context* ctx, const uint8_t* key, size_t keyLen) {
  // keyLen em bytes, mbedtls espera bits
  return mbedtls_gcm_setkey(ctx, MBEDTLS_CIPHER_ID_AES, key, (unsigned int)(keyLen * 8)) == 0;
}

AesGcmEncrypted AesGcmCodec::encrypt(
    const uint8_t* key, size_t keyLen,
    const uint8_t* iv, size_t ivLen,
    const String& plaintext) {
  return encryptWithAad(key, keyLen, iv, ivLen, nullptr, 0, plaintext);
}

AesGcmEncrypted AesGcmCodec::encryptWithAad(
    const uint8_t* key, size_t keyLen,
    const uint8_t* iv, size_t ivLen,
    const uint8_t* aad, size_t aadLen,
    const String& plaintext) {

  AesGcmEncrypted out;
  if (!key || !iv) return out;
  if (keyLen != 32 || ivLen != 12) return out;

  const size_t n = plaintext.length();
  if (n == 0) return out;

  std::unique_ptr<uint8_t[]> cipher(new uint8_t[n]);
  uint8_t tag[16];

  mbedtls_gcm_context ctx;
  mbedtls_gcm_init(&ctx);

  if (!setKey(&ctx, key, keyLen)) {
    mbedtls_gcm_free(&ctx);
    return out;
  }

  int ret = mbedtls_gcm_crypt_and_tag(
      &ctx,
      MBEDTLS_GCM_ENCRYPT,
      n,
      iv, ivLen,
      aad, aadLen,
      (const uint8_t*)plaintext.c_str(),
      cipher.get(),
      sizeof(tag),
      tag
  );

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
  return decryptWithAad(key, keyLen, iv, ivLen, nullptr, 0, cipher, cipherLen, tag, tagLen);
}

String AesGcmCodec::decryptWithAad(
    const uint8_t* key, size_t keyLen,
    const uint8_t* iv, size_t ivLen,
    const uint8_t* aad, size_t aadLen,
    const uint8_t* cipher, size_t cipherLen,
    const uint8_t* tag, size_t tagLen) {

  if (!key || !iv || !cipher || !tag) return "";
  if (keyLen != 32 || ivLen != 12 || tagLen != 16) return "";

  std::unique_ptr<uint8_t[]> plain(new uint8_t[cipherLen + 1]);
  plain.get()[cipherLen] = 0;

  mbedtls_gcm_context ctx;
  mbedtls_gcm_init(&ctx);

  if (!setKey(&ctx, key, keyLen)) {
    mbedtls_gcm_free(&ctx);
    return "";
  }

  int ret = mbedtls_gcm_auth_decrypt(
      &ctx, cipherLen,
      iv, ivLen,
      aad, aadLen,
      tag, tagLen,
      cipher,
      plain.get()
  );

  mbedtls_gcm_free(&ctx);
  if (ret != 0) return "";

  // String(const char*) para dados binários pode cortar em \0.
  // Aqui esperamos JSON ASCII, então OK. Usamos length explícito para segurança.
  return String((const char*)plain.get()).substring(0, cipherLen);
}
