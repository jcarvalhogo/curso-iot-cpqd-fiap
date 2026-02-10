# SecureHttp

`SecureHttp` é uma biblioteca para **ESP32 + Arduino** que adiciona segurança sobre HTTP (sem TLS) para uso em **rede local**.

Fornece:
- **AES-256-GCM**: confidencialidade + integridade do payload
- **HMAC-SHA256**: autenticação do request (evita forja)
- **Anti-replay**: janela de timestamp + cache de nonce

> Não substitui HTTPS/TLS em tráfego exposto à internet.

## Como funciona (visão técnica)

### 1) Segredos compartilhados
Device e gateway compartilham a configuração em `SecureHttpConfig.h` (não commitar):
- `SECUREHTTP_AES256_KEY` (32 bytes)
- `SECUREHTTP_HMAC_KEY` e `SECUREHTTP_HMAC_KEY_LEN`
- `SECURE_DEVICE_ID`

### 2) Envelope do request

**Body**
- `ciphertextHex`: ciphertext em hexadecimal

**Headers**
- `X-Device-Id`
- `X-Timestamp` (epoch em segundos)
- `X-Nonce` (hex)
- `X-IV` (12 bytes em hex)
- `X-Tag` (16 bytes em hex)
- `X-Signature` (HMAC-SHA256 em hex)

### 3) AAD do AES-GCM

A decriptação só é válida se o **AAD** bater exatamente:

```
aad = deviceId + "|" + timestamp + "|" + nonce + "|" + method + "|" + path
```

### 4) Texto canônico assinado (HMAC)

Device e gateway calculam o HMAC deste texto (mesmo formato nos dois lados):

```
METHOD\nPATH\nDEVICE\nTS\nNONCE\nIV\nTAG\nCIPHERTEXT_HEX
```

## Requisito importante: relógio (NTP)

`X-Timestamp` é **epoch real**. Portanto, **device e gateway precisam sincronizar hora via NTP/SNTP**.
Sem isso, é comum ver `timestamp_out_of_window`.

## Como usar

### Gateway (verificar e decriptar)
```cpp
#include <SecureGatewayAuth.h>
static SecureGatewayAuth auth;

auto res = auth.verifyAndDecrypt(server, "POST", "/telemetry");
if (!res.ok) {
  server.send(res.httpCode, "application/json",
              "{\"ok\":false,\"error\":\"" + res.error + "\"}");
  return;
}
String plaintextJson = res.plaintextJson;
```

### Device (cifrar e assinar)
```cpp
#include <SecureDeviceAuth.h>
#include <SecureHttpConfig.h>

SecureDeviceAuth auth;
auto r = auth.encryptAndSign(SECURE_DEVICE_ID, "POST", "/telemetry", plaintextJson);
if (!r.ok) return;
// Envie body=r.ciphertextHex e os headers de r
```

## Erros comuns

Gateway (`SecureGatewayAuth`):
- `missing_headers`, `unknown_device`, `bad_timestamp`, `timestamp_out_of_window`
- `replay_nonce`, `bad_body`, `bad_signature`, `bad_iv_or_tag`, `decrypt_failed`

Device (`SecureDeviceAuth`):
- `time_not_synced`, `iv_gen_failed`, `encrypt_failed`, `hmac_failed`

## Documentação (Doxygen)

A lib inclui um `docs/Doxyfile` pronto.

```bash
cd SecureHttp
doxygen docs/Doxyfile
# saída: docs/out/html/index.html
```

