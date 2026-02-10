# SecureHttp

Library for ESP32 Arduino projects that adds confidentiality and request authentication over plain HTTP:

- AES-256-GCM encryption/decryption (payload confidentiality + integrity)
- HMAC-SHA256 signature (request authentication)
- Timestamp window + nonce cache (anti-replay)

## Integration (gateway)

1. Add this library under `lib/SecureHttp/`.
2. Create `include/SecureHttpConfig.h` by copying `include/SecureHttpConfig.example.h` and filling secrets.
3. In your HTTP route handler (e.g. `/telemetry`), verify and decrypt:

```cpp
#include <SecureGatewayAuth.h>

static SecureGatewayAuth secureAuth;

auto res = secureAuth.verifyAndDecrypt(server, "POST", "/telemetry");
if (!res.ok) {
  server.send(res.httpCode, "application/json", "{\"ok\":false,\"error\":\"" + res.error + "\"}");
  return;
}

String plaintextJson = res.plaintextJson;
// parse JSON + publish
```

## Protocol

Request body: ciphertext as HEX string.

Headers:
- X-Device-Id
- X-Timestamp (uint seconds)
- X-Nonce
- X-IV (12 bytes hex)
- X-Tag (16 bytes hex)
- X-Signature (HMAC-SHA256 hex)

Canonical string for HMAC:
`deviceId + "\\n" + timestamp + "\\n" + nonce + "\\n" + method + "\\n" + path + "\\n" + sha256Hex(bodyCipherHex)`


## Documentation
Generate API docs with Doxygen using `docs/Doxyfile` (see `docs/BUILD_DOCS.md`).
