//
// Created by Josemar Carvalho on 07/02/26.
//

#include "HttpServer.h"

HttpServer::HttpServer(uint16_t port)
    : _server(port) {
}

void HttpServer::begin() {
    registerRoutes();
    _server.begin();

    // Libera o 1º envio do ThingSpeak assim que chegar o 1º dado válido
    _lastThingSpeakSendMs = 0;

    Serial.println("[HTTP] Server started");
}

void HttpServer::update() {
    _server.handleClient();
    tickThingSpeakTimer(); // timer do ThingSpeak roda no loop
}

const HttpServer::Telemetry &HttpServer::telemetry() const {
    return _telemetry;
}

// Ubidots (imediato)
void HttpServer::onTelemetryUpdated(TelemetryCallback cb) {
    _onTelemetryUpdated = cb;
}

// ThingSpeak (a cada 30s)
void HttpServer::onThingSpeakDue(TelemetryCallback cb) {
    _onThingSpeakDue = cb;
}

void HttpServer::setThingSpeakIntervalMs(uint32_t intervalMs) {
    if (intervalMs < 1000) intervalMs = 1000; // proteção mínima
    _thingSpeakIntervalMs = intervalMs;
}

bool HttpServer::canSendThingSpeakNow() const {
    const uint32_t now = millis();
    const uint32_t elapsed = now - _lastThingSpeakSendMs; // ok com overflow
    return elapsed >= _thingSpeakIntervalMs;
}

void HttpServer::markThingSpeakSent() {
    _lastThingSpeakSendMs = millis();
    _thingSpeakPending = false;
}

void HttpServer::registerRoutes() {
    _server.on("/", HTTP_GET, [this]() { handleRoot(); });

    _server.on("/telemetry", HTTP_GET, [this]() { handleTelemetryGet(); });
    _server.on("/telemetry", HTTP_POST, [this]() { handleTelemetryPost(); });

    _server.onNotFound([this]() { handleNotFound(); });
}

void HttpServer::handleRoot() {
    _server.send(200, "text/plain",
                 "gateway-arduino\n"
                 "GET  /telemetry\n"
                 "POST /telemetry (SecureHttp)\n"
                 "\n"
                 "POST /telemetry expects:\n"
                 "  - Body: ciphertext HEX (AES-256-GCM)\n"
                 "  - Headers: X-Device-Id, X-Timestamp, X-Nonce, X-IV, X-Tag, X-Signature\n");
}

void HttpServer::handleTelemetryGet() {
    _server.send(200, "application/json", makeTelemetryJson(_telemetry));
}

void HttpServer::handleTelemetryPost() {
    // SecureHttp:
    // - body: ciphertext hex string (NOT JSON)
    // - headers: X-Device-Id, X-Timestamp, X-Nonce, X-IV, X-Tag, X-Signature
    // Result: plaintext JSON in res.plaintextJson

    auto res = _secureAuth.verifyAndDecrypt(_server, "POST", "/telemetry");
    if (!res.ok) {
        _server.send(res.httpCode, "application/json",
                     "{\"ok\":false,\"error\":\"" + res.error + "\"}");
        return;
    }

    String body = res.plaintextJson;
    body.trim();

    bool okTemp = false;
    bool okHum = false;
    bool okFuel = false;
    bool okSpeed = false;
    bool okRpm = false;

    float temp = NAN;
    float hum = NAN;

    float fuelF = NAN;
    float speed = NAN;
    float rpm = NAN;

    if (body.length() > 0) {
        okTemp = tryExtractJsonNumber(body, "temperature", temp);
        okHum = tryExtractJsonNumber(body, "humidity", hum);
        okFuel = tryExtractJsonNumber(body, "fuelLevel", fuelF);
        okSpeed = tryExtractJsonNumber(body, "stepperSpeed", speed);
        okRpm = tryExtractJsonNumber(body, "stepperRpm", rpm);
    }

    // If nothing came, reject
    if (!okTemp && !okHum && !okFuel && !okSpeed && !okRpm) {
        _server.send(400, "application/json",
                     "{\"ok\":false,\"error\":\"Missing telemetry fields\",\"hint\":\"Send encrypted JSON with fields: temperature, humidity, fuelLevel, stepperSpeed, stepperRpm\"}");
        return;
    }

    // Update stored telemetry (only update fields present)
    if (okTemp) _telemetry.temperature = temp;
    if (okHum) _telemetry.humidity = hum;

    if (okFuel) {
        // aceita int no payload (vem como float), e valida range simples 0..100
        int fl = (int) fuelF;
        if (fl < 0) fl = 0;
        if (fl > 100) fl = 100;
        _telemetry.fuelLevel = fl;
    }

    if (okSpeed) _telemetry.stepperSpeed = speed;
    if (okRpm) _telemetry.stepperRpm = rpm;

    _telemetry.counter++;
    _telemetry.lastUpdateMs = millis();

    // Mantém seu comportamento: hasData=true somente quando temp e hum são válidos
    const bool hasTemp = !isnan(_telemetry.temperature);
    const bool hasHum = !isnan(_telemetry.humidity);
    _telemetry.hasData = hasTemp && hasHum;

    // Ubidots: envia imediatamente quando telemetria é publicável
    if (_telemetry.hasData && _onTelemetryUpdated) {
        _onTelemetryUpdated(_telemetry);
    }

    // ThingSpeak: marca como pendente (timer decide quando enviar)
    if (_telemetry.hasData) {
        _thingSpeakPending = true;
    }

    // Reply com debug + telemetry (não ecoa plaintext recebido)
    String resp = "{";
    resp += "\"ok\":true";
    resp += ",\"updated\":{";
    resp += "\"temperature\":" + String(okTemp ? "true" : "false");
    resp += ",\"humidity\":" + String(okHum ? "true" : "false");
    resp += ",\"fuelLevel\":" + String(okFuel ? "true" : "false");
    resp += ",\"stepperSpeed\":" + String(okSpeed ? "true" : "false");
    resp += ",\"stepperRpm\":" + String(okRpm ? "true" : "false");
    resp += "}";
    resp += ",\"telemetry\":" + makeTelemetryJson(_telemetry);
    resp += "}";

    _server.send(200, "application/json", resp);
}

void HttpServer::tickThingSpeakTimer() {
    // Só envia se tiver callback, dado válido e existir algo pendente
    if (!_onThingSpeakDue) return;
    if (!_thingSpeakPending) return;
    if (!_telemetry.hasData) return;

    if (!canSendThingSpeakNow()) return;

    // marca antes para evitar duplicidade/reentrância
    markThingSpeakSent();
    _onThingSpeakDue(_telemetry);
}

void HttpServer::handleNotFound() {
    String msg = "{\"error\":\"Not found\",\"path\":\"" + _server.uri() + "\"}";
    _server.send(404, "application/json", msg);
}

// Minimal JSON number extractor for simple payloads.
// Example: {"temperature": 25.3, "humidity": 50}
bool HttpServer::tryExtractJsonNumber(const String &json, const char *key, float &out) {
    String needle = String("\"") + key + String("\"");
    int k = json.indexOf(needle);
    if (k < 0) return false;

    int colon = json.indexOf(':', k + needle.length());
    if (colon < 0) return false;

    int i = colon + 1;
    while (i < (int) json.length() && isspace((unsigned char) json[i])) i++;

    int j = i;
    while (j < (int) json.length()) {
        char c = json[j];
        if (c == ',' || c == '}' || isspace((unsigned char) json[j])) break;
        j++;
    }

    if (j <= i) return false;

    String num = json.substring(i, j);
    num.trim();
    if (num.isEmpty()) return false;

    out = num.toFloat();
    return true;
}

String HttpServer::makeTelemetryJson(const Telemetry &t) {
    String s = "{";
    s += "\"hasData\":" + String(t.hasData ? "true" : "false");
    s += ",\"temperature\":" + (isnan(t.temperature) ? String("null") : String(t.temperature, 2));
    s += ",\"humidity\":" + (isnan(t.humidity) ? String("null") : String(t.humidity, 2));

    // novos campos
    s += ",\"fuelLevel\":" + String(t.fuelLevel < 0 ? "null" : String(t.fuelLevel));
    s += ",\"stepperSpeed\":" + (isnan(t.stepperSpeed) ? String("null") : String(t.stepperSpeed, 1));
    s += ",\"stepperRpm\":" + (isnan(t.stepperRpm) ? String("null") : String(t.stepperRpm, 2));

    s += ",\"counter\":" + String(t.counter);
    s += ",\"lastUpdateMs\":" + String(t.lastUpdateMs);
    s += "}";
    return s;
}
