#include "GatewayClient.h"

#include <WiFi.h>
#include <WiFiClient.h>

static void dbgNet(Stream *dbg, const char *host, uint16_t port) {
    if (!dbg) return;
    dbg->print("[Net] local=");
    dbg->print(WiFi.localIP());
    dbg->print(" rssi=");
    dbg->print(WiFi.RSSI());
    dbg->print(" dst=");
    dbg->print(host ? host : "(null)");
    dbg->print(":");
    dbg->println(port);
}

GatewayClient::GatewayClient(const Config &cfg) : _cfg(cfg) {
}

void GatewayClient::begin() {
    // reservado (se no futuro SecureDeviceAuth tiver begin/init, pode chamar aqui)
}

void GatewayClient::update() {
    // reservado
}

void GatewayClient::setDebugStream(Stream *s) {
    _dbg = s;
}

void GatewayClient::dbgln(const String &s) {
    if (_dbg) _dbg->println(s);
}

bool GatewayClient::isConfigValid() const {
    return _cfg.host && _cfg.host[0] != '\0' &&
           _cfg.port != 0 &&
           _cfg.path && _cfg.path[0] != '\0' &&
           _cfg.deviceId && _cfg.deviceId[0] != '\0';
}

bool GatewayClient::canPublishNow(uint32_t now) const {
    if (_lastPublishMs == 0) return true;
    return (now - _lastPublishMs) >= _cfg.minIntervalMs;
}

bool GatewayClient::publishTelemetry(float temperature, float humidity) {
    return publishTelemetry(temperature, humidity, -1, -1.0f, -1.0f);
}

bool GatewayClient::publishTelemetry(float temperature, float humidity, int fuelLevelPercent) {
    return publishTelemetry(temperature, humidity, fuelLevelPercent, -1.0f, -1.0f);
}

bool GatewayClient::publishTelemetry(float temperature, float humidity, int fuelLevelPercent, float stepperSpeed) {
    return publishTelemetry(temperature, humidity, fuelLevelPercent, stepperSpeed, -1.0f);
}

bool GatewayClient::publishTelemetry(float temperature, float humidity,
                                     int fuelLevelPercent,
                                     float stepperSpeed,
                                     float stepperRpm) {
    _lastError = Error::None;
    _lastHttpStatus = -1;

    if (!isConfigValid()) {
        _lastError = Error::InvalidConfig;
        dbgln("[Gateway] invalid config (host/port/path/deviceId)");
        return false;
    }

    const uint32_t now = millis();
    if (!canPublishNow(now)) {
        _lastError = Error::RateLimited;
        dbgln("[Gateway] rate-limited (min interval not reached)");
        return false;
    }

    if (WiFi.status() != WL_CONNECTED) {
        _lastError = Error::WifiNotConnected;
        dbgln("[Gateway] WiFi not connected");
        return false;
    }

    // plaintext JSON (será criptografado)
    String body = "{";
    body += "\"temperature\":" + String(temperature, 2);
    body += ",\"humidity\":" + String(humidity, 2);

    if (fuelLevelPercent >= 0) {
        fuelLevelPercent = constrain(fuelLevelPercent, 0, 100);
        body += ",\"fuelLevel\":" + String(fuelLevelPercent);
    }
    if (stepperSpeed >= 0.0f) body += ",\"stepperSpeed\":" + String(stepperSpeed, 1);
    if (stepperRpm >= 0.0f) body += ",\"stepperRpm\":" + String(stepperRpm, 2);

    body += "}";

    const bool ok = sendSecurePost(body);
    if (ok) _lastPublishMs = now;
    return ok;
}

bool GatewayClient::sendSecurePost(const String &plaintextJson) {
    auto req = _secure.encryptAndSign(_cfg.deviceId, "POST", _cfg.path, plaintextJson);
    if (!req.ok) {
        _lastError = Error::SecureBuildFailed;
        dbgln(String("[Gateway] secure build failed: ") + req.error);
        return false;
    }

    WiFiClient client;

    // "best effort" (Arduino core), a gente controla timeout no loop abaixo
    client.setTimeout(1);

    bool connected = false;
    for (int attempt = 1; attempt <= 2; attempt++) {
        if (client.connect(_cfg.host, _cfg.port)) {
            connected = true;
            break;
        }
        if (attempt == 1) {
            dbgln("[Gateway] connect failed");
            dbgNet(_dbg, _cfg.host, _cfg.port);
        }
        delay(50);
    }

    if (!connected) {
        _lastError = Error::ConnectFailed;
        return false;
    }

    const String &cipherHex = req.ciphertextHex;

    // HTTP POST SecureHttp
    client.print(String("POST ") + _cfg.path + " HTTP/1.1\r\n");
    client.print(String("Host: ") + _cfg.host + "\r\n");
    client.print("User-Agent: vehicle-device/1.0\r\n");
    client.print("Content-Type: application/octet-stream\r\n");
    client.print(String("Content-Length: ") + cipherHex.length() + "\r\n");

    client.print(String("X-Device-Id: ") + req.deviceId + "\r\n");
    client.print(String("X-Timestamp: ") + req.timestamp + "\r\n");
    client.print(String("X-Nonce: ") + req.nonce + "\r\n");
    client.print(String("X-IV: ") + req.ivHex + "\r\n");
    client.print(String("X-Tag: ") + req.tagHex + "\r\n");
    client.print(String("X-Signature: ") + req.signatureHex + "\r\n");

    client.print("Connection: close\r\n\r\n");
    client.print(cipherHex);

    // Aguarda status line
    const uint32_t t0 = millis();
    while (!client.available()) {
        if (millis() - t0 > _cfg.timeoutMs) {
            _lastError = Error::Timeout;
            dbgln("[Gateway] timeout waiting response");
            client.stop();
            return false;
        }
        delay(2);
    }

    String statusLine = client.readStringUntil('\n');
    statusLine.trim();

    // Parse "HTTP/1.1 200 OK"
    int code = -1;
    int p1 = statusLine.indexOf(' ');
    if (p1 > 0) {
        int p2 = statusLine.indexOf(' ', p1 + 1);
        if (p2 > p1) code = statusLine.substring(p1 + 1, p2).toInt();
    }
    _lastHttpStatus = code;

    // Consome headers
    while (client.available()) {
        String line = client.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) break;
    }

    // Lê body (útil em erro)
    String respBody;
    const uint32_t t1 = millis();
    while (millis() - t1 < 250 && client.available()) {
        respBody += client.readString();
    }

    client.stop();

    if (code < 200 || code >= 300) {
        _lastError = Error::BadHttpStatus;
        dbgln(String("[Gateway] bad HTTP status=") + code + " resp=" + respBody);
        return false;
    }

    _lastError = Error::None;
    dbgln(String("[Gateway] OK HTTP=") + code);
    return true;
}
