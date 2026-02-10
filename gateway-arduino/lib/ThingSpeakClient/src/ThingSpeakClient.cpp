//
// Created by Josemar Carvalho on 08/02/26.
//

#include "ThingSpeakClient.h"

#include <WiFi.h>
#include <ctype.h>

ThingSpeakClient::ThingSpeakClient(const Config &cfg) : _cfg(cfg) {
}

void ThingSpeakClient::begin() {
    // reservado (mantém simetria com outras libs)
}

void ThingSpeakClient::update() {
    // reservado
}

void ThingSpeakClient::setDebugStream(Stream *s) {
    _dbg = s;
}

void ThingSpeakClient::dbgln(const String &s) {
    if (_dbg) _dbg->println(s);
}

bool ThingSpeakClient::canPublishNow(uint32_t now) const {
    if (_lastPublishMs == 0) return true;
    return (now - _lastPublishMs) >= _cfg.minIntervalMs;
}

bool ThingSpeakClient::telemetryIsPublishable(const HttpServer::Telemetry &t) const {
    if (!t.hasData) return false;

    const bool hasTemp = !isnan(t.temperature);
    const bool hasHum = !isnan(t.humidity);

    // Para payload novo, os extras podem ser opcionais (dependendo do allowPartialTelemetry)
    const bool hasAnyExtra =
            (t.fuelLevel >= 0) ||
            (!isnan(t.stepperSpeed)) ||
            (!isnan(t.stepperRpm));

    if (_cfg.allowPartialTelemetry) {
        // se parcial: precisa ter pelo menos um campo válido (temp/hum/extras)
        return hasTemp || hasHum || hasAnyExtra;
    }

    // se não parcial: mantém o comportamento antigo (exige temp + hum)
    return hasTemp && hasHum;
}

bool ThingSpeakClient::publishTelemetry(const HttpServer::Telemetry &t) {
    _lastError = Error::None;

    if (!_cfg.isValid()) {
        _lastError = Error::InvalidConfig;
        dbgln("[ThingSpeak] invalid config (missing key/host/port)");
        return false;
    }

    if (!telemetryIsPublishable(t)) {
        _lastError = Error::InvalidTelemetry;
        dbgln("[ThingSpeak] invalid telemetry (missing fields)");
        return false;
    }

    const float temp = (!isnan(t.temperature)) ? t.temperature : NAN;
    const float hum = (!isnan(t.humidity)) ? t.humidity : NAN;

    // extras (opcionais)
    const int fuel = t.fuelLevel; // usar <0 para omitir
    const float speed = (!isnan(t.stepperSpeed)) ? t.stepperSpeed : NAN;
    const float rpm = (!isnan(t.stepperRpm)) ? t.stepperRpm : NAN;

    return publish(temp, hum, fuel, speed, rpm);
}

bool ThingSpeakClient::publish(float temperature, float humidity) {
    // mantém compatibilidade (somente temp/hum)
    return publish(temperature, humidity, -1, NAN, NAN);
}

bool ThingSpeakClient::publish(float temperature,
                               float humidity,
                               int fuelLevel,
                               float stepperSpeed,
                               float stepperRpm) {
    _lastHttpStatus = -1;
    _lastEntryId = 0;

    // Validação mínima
    if (!_cfg.allowPartialTelemetry) {
        // exige pelo menos temp + hum (comportamento antigo)
        if (isnan(temperature) || isnan(humidity)) {
            _lastError = Error::InvalidTelemetry;
            dbgln("[ThingSpeak] invalid telemetry (NaN fields)");
            return false;
        }
    } else {
        // parcial: precisa ter pelo menos um campo válido entre os 5
        const bool any =
                (!isnan(temperature)) ||
                (!isnan(humidity)) ||
                (fuelLevel >= 0) ||
                (!isnan(stepperSpeed)) ||
                (!isnan(stepperRpm));

        if (!any) {
            _lastError = Error::InvalidTelemetry;
            dbgln("[ThingSpeak] invalid telemetry (all fields empty)");
            return false;
        }
    }

    const uint32_t now = millis();
    if (!canPublishNow(now)) {
        _lastError = Error::RateLimited;
        dbgln("[ThingSpeak] rate-limited (min interval not reached)");
        return false;
    }

    if (WiFi.status() != WL_CONNECTED) {
        _lastError = Error::WifiNotConnected;
        dbgln("[ThingSpeak] WiFi not connected");
        return false;
    }

    WiFiClient client;
    if (!client.connect(_cfg.host, _cfg.port)) {
        _lastError = Error::ConnectFailed;
        dbgln("[ThingSpeak] connect failed");
        return false;
    }

    // Monta URL:
    // /update?api_key=...&field1=...&field2=...&field3=...&field4=...&field5=...
    String url = "/update?api_key=";
    url += _cfg.writeApiKey;

    if (!isnan(temperature)) {
        url += "&field1=";
        url += String(temperature, 2);
    }
    if (!isnan(humidity)) {
        url += "&field2=";
        url += String(humidity, 2);
    }
    if (fuelLevel >= 0) {
        url += "&field3=";
        url += String(fuelLevel);
    }
    if (!isnan(stepperSpeed)) {
        url += "&field4=";
        url += String(stepperSpeed, 1);
    }
    if (!isnan(stepperRpm)) {
        url += "&field5=";
        url += String(stepperRpm, 2);
    }

    client.print(String("GET ") + url + " HTTP/1.1\r\n");
    client.print(String("Host: ") + _cfg.host + "\r\n");
    client.print("Connection: close\r\n\r\n");

    const uint32_t timeoutMs = 4500;
    uint32_t t0 = millis();

    while (!client.available()) {
        if (millis() - t0 > timeoutMs) {
            _lastError = Error::Timeout;
            dbgln("[ThingSpeak] timeout waiting response");
            client.stop();
            return false;
        }
        delay(10);
    }

    // Status line: HTTP/1.1 200 OK
    String statusLine = client.readStringUntil('\n');
    statusLine.trim();

    int code = -1;
    int p1 = statusLine.indexOf(' ');
    if (p1 > 0) {
        int p2 = statusLine.indexOf(' ', p1 + 1);
        if (p2 > p1) code = statusLine.substring(p1 + 1, p2).toInt();
    }
    _lastHttpStatus = code;

    // Headers -> body
    bool inBody = false;
    String body;

    while (client.connected() || client.available()) {
        if (millis() - t0 > timeoutMs) {
            _lastError = Error::Timeout;
            dbgln("[ThingSpeak] timeout reading response");
            break;
        }

        if (!client.available()) {
            delay(10);
            continue;
        }

        String line = client.readStringUntil('\n');
        line.trim();

        if (!inBody) {
            if (line.length() == 0) inBody = true;
            continue;
        } else {
            if (line.length()) body += line;
        }
    }

    client.stop();
    body.trim();

    if (code != 200) {
        _lastError = Error::HttpBadStatus;
        dbgln(String("[ThingSpeak] HTTP status=") + code + " body=" + body);
        return false;
    }

    long entryId = body.toInt();
    _lastEntryId = entryId;

    if (entryId <= 0) {
        _lastError = Error::WriteFailed;
        dbgln(String("[ThingSpeak] write failed (entry_id<=0). body=") + body);
        return false;
    }

    _lastPublishMs = now;
    dbgln(String("[ThingSpeak] OK entry_id=") + entryId);
    return true;
}

ThingSpeakClient::Error ThingSpeakClient::lastError() const noexcept {
    return _lastError;
}

int ThingSpeakClient::lastHttpStatus() const noexcept {
    return _lastHttpStatus;
}

long ThingSpeakClient::lastEntryId() const noexcept {
    return _lastEntryId;
}

uint32_t ThingSpeakClient::lastPublishMs() const noexcept {
    return _lastPublishMs;
}

String ThingSpeakClient::maskKey(const char *key) {
    if (!key) return "";
    String k(key);
    if (k.length() <= 8) return "********";
    return k.substring(0, 4) + "****" + k.substring(k.length() - 4);
}
