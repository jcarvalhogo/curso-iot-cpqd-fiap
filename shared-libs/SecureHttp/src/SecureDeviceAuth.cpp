//
// Created by Josemar Carvalho on 10/02/26.
//

#include "SecureDeviceAuth.h"

#include <time.h>
#include <cstring>
#include <memory>

#include <mbedtls/gcm.h>
#include <mbedtls/md.h>
#include <esp_system.h>

#include "SecureHttpConfig.h"

#ifndef SECUREHTTP_AES256_KEY
#error "SECUREHTTP_AES256_KEY not defined in SecureHttpConfig.h"
#endif

#ifndef SECUREHTTP_HMAC_KEY
#error "SECUREHTTP_HMAC_KEY not defined in SecureHttpConfig.h"
#endif

#ifndef SECUREHTTP_HMAC_KEY_LEN
#error "SECUREHTTP_HMAC_KEY_LEN not defined in SecureHttpConfig.h"
#endif

static uint8_t hexNibble(char c) {
    if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
    if (c >= 'a' && c <= 'f') return (uint8_t)(10 + (c - 'a'));
    if (c >= 'A' && c <= 'F') return (uint8_t)(10 + (c - 'A'));
    return 0xFF;
}

String SecureDeviceAuth::toHex(const uint8_t *data, size_t len) {
    static const char *hex = "0123456789abcdef";
    String out;
    out.reserve(len * 2);
    for (size_t i = 0; i < len; i++) {
        out += hex[(data[i] >> 4) & 0x0F];
        out += hex[data[i] & 0x0F];
    }
    return out;
}

bool SecureDeviceAuth::fromHex(const String &hex, uint8_t *out, size_t outLen) {
    if ((hex.length() % 2) != 0) return false;
    const size_t want = (size_t)hex.length() / 2;
    if (want != outLen) return false;

    for (size_t i = 0; i < outLen; i++) {
        const uint8_t hi = hexNibble(hex[2 * i]);
        const uint8_t lo = hexNibble(hex[2 * i + 1]);
        if (hi == 0xFF || lo == 0xFF) return false;
        out[i] = (uint8_t)((hi << 4) | lo);
    }
    return true;
}

String SecureDeviceAuth::randomHex(size_t bytesLen) {
    uint8_t buf[32];
    if (bytesLen > sizeof(buf)) bytesLen = sizeof(buf);
    for (size_t i = 0; i < bytesLen; i += 4) {
        uint32_t r = esp_random();
        size_t n = (bytesLen - i >= 4) ? 4 : (bytesLen - i);
        memcpy(&buf[i], &r, n);
    }
    return toHex(buf, bytesLen);
}

bool SecureDeviceAuth::aes256gcmEncrypt(const uint8_t *key32,
                                       const uint8_t *iv12,
                                       const uint8_t *aad, size_t aadLen,
                                       const uint8_t *pt, size_t ptLen,
                                       uint8_t *ct,
                                       uint8_t *tag16) {
    mbedtls_gcm_context gcm;
    mbedtls_gcm_init(&gcm);

    int rc = mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key32, 256);
    if (rc != 0) { mbedtls_gcm_free(&gcm); return false; }

    rc = mbedtls_gcm_crypt_and_tag(&gcm,
                                  MBEDTLS_GCM_ENCRYPT,
                                  ptLen,
                                  iv12, 12,
                                  aad, aadLen,
                                  pt,
                                  ct,
                                  16, tag16);

    mbedtls_gcm_free(&gcm);
    return rc == 0;
}

bool SecureDeviceAuth::hmacSha256Hex(const uint8_t *key, size_t keyLen,
                                     const uint8_t *msg, size_t msgLen,
                                     String &outHex) {
    const mbedtls_md_info_t *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (!info) return false;

    uint8_t mac[32];
    int rc = mbedtls_md_hmac(info, key, keyLen, msg, msgLen, mac);
    if (rc != 0) return false;

    outHex = toHex(mac, sizeof(mac));
    return true;
}

String SecureDeviceAuth::canonicalToSign(const char *method,
                                         const char *path,
                                         const String &deviceId,
                                         const String &timestamp,
                                         const String &nonce,
                                         const String &ivHex,
                                         const String &tagHex,
                                         const String &ciphertextHex) {
    // MUST match gateway exactly:
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

SecureDeviceAuth::Result SecureDeviceAuth::encryptAndSign(const char *deviceId,
                                                          const char *method,
                                                          const char *path,
                                                          const String &plaintextJson) const {
    Result r;

    if (!deviceId || !*deviceId) { r.error = "invalid_device_id"; return r; }
    if (!method || !*method) { r.error = "invalid_method"; return r; }
    if (!path || !*path) { r.error = "invalid_path"; return r; }
    if (plaintextJson.length() == 0) { r.error = "empty_body"; return r; }

    time_t now = time(nullptr);
    if (now < 1700000000) {
        r.error = "time_not_synced";
        return r;
    }

    r.deviceId = deviceId;
    r.timestamp = String((uint32_t)now);
    r.nonce = randomHex(8);

    uint8_t iv[12];
    {
        String ivHex = randomHex(12);
        if (!fromHex(ivHex, iv, sizeof(iv))) { r.error = "iv_gen_failed"; return r; }
        r.ivHex = ivHex;
    }

    // AAD MUST match gateway
    String aadStr;
    aadStr.reserve(64);
    aadStr += r.deviceId;  aadStr += "|";
    aadStr += r.timestamp; aadStr += "|";
    aadStr += r.nonce;     aadStr += "|";
    aadStr += method;      aadStr += "|";
    aadStr += path;

    const uint8_t *pt = (const uint8_t *)plaintextJson.c_str();
    const size_t ptLen = plaintextJson.length();

    std::unique_ptr<uint8_t[]> ct(new uint8_t[ptLen]);
    uint8_t tag[16];

    if (!aes256gcmEncrypt(SECUREHTTP_AES256_KEY, iv,
                          (const uint8_t*)aadStr.c_str(), aadStr.length(),
                          pt, ptLen,
                          ct.get(),
                          tag)) {
        r.error = "encrypt_failed";
        return r;
    }

    r.ciphertextHex = toHex(ct.get(), ptLen);
    r.tagHex = toHex(tag, sizeof(tag));

    const String canonical = canonicalToSign(method, path,
                                             r.deviceId, r.timestamp, r.nonce,
                                             r.ivHex, r.tagHex, r.ciphertextHex);

    String sigHex;
    if (!hmacSha256Hex(SECUREHTTP_HMAC_KEY, SECUREHTTP_HMAC_KEY_LEN,
                       (const uint8_t*)canonical.c_str(), canonical.length(),
                       sigHex)) {
        r.error = "hmac_failed";
        return r;
    }

    r.signatureHex = sigHex;
    r.ok = true;
    return r;
}