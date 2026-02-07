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
    Serial.println("[HTTP] Server started");
}

void HttpServer::update() {
    _server.handleClient();
}

const HttpServer::Telemetry &HttpServer::telemetry() const {
    return _telemetry;
}

void HttpServer::onTelemetryUpdated(TelemetryCallback cb) {
    _onTelemetryUpdated = cb;
}

void HttpServer::registerRoutes() {
    _server.on("/", HTTP_GET, [this]() { handleRoot(); });

    // English endpoint name as requested:
    _server.on("/telemetry", HTTP_GET, [this]() { handleTelemetryGet(); });
    _server.on("/telemetry", HTTP_POST, [this]() { handleTelemetryPost(); });

    _server.onNotFound([this]() { handleNotFound(); });
}

void HttpServer::handleRoot() {
    _server.send(200, "text/plain",
                 "gateway-arduino\n"
                 "GET  /telemetry\n"
                 "POST /telemetry\n");
}

void HttpServer::handleTelemetryGet() {
    _server.send(200, "application/json", makeTelemetryJson(_telemetry));
}

void HttpServer::handleTelemetryPost() {
    // Accept payload from:
    // 1) JSON body: {"temperature": 25.3, "humidity": 50.1}
    // 2) query/form args: temperature=...&humidity=...

    bool okTemp = false;
    bool okHum = false;

    float temp = NAN;
    float hum = NAN;

    // First try args (works for form-url-encoded and query string)
    okTemp = tryReadFloatArg(_server, "temperature", temp);
    okHum = tryReadFloatArg(_server, "humidity", hum);

    // Then try JSON body if still missing
    if (!okTemp || !okHum) {
        // Body is available via plain() on ESP32 WebServer
        String body = _server.arg("plain");
        body.trim();

        if (body.length() > 0) {
            if (!okTemp) okTemp = tryExtractJsonNumber(body, "temperature", temp);
            if (!okHum) okHum = tryExtractJsonNumber(body, "humidity", hum);
        }
    }

    if (!okTemp && !okHum) {
        _server.send(400, "application/json",
                     "{\"error\":\"Missing telemetry fields. Send JSON body or args: temperature, humidity\"}");
        return;
    }

    // Update stored telemetry (only update fields present)
    if (okTemp) _telemetry.temperature = temp;
    if (okHum) _telemetry.humidity = hum;

    _telemetry.hasData = true;
    _telemetry.counter++;
    _telemetry.lastUpdateMs = millis();

    // Notify listeners (e.g., publish to Ubidots) AFTER state update
    if (_onTelemetryUpdated) {
        _onTelemetryUpdated(_telemetry);
    }

    _server.send(200, "application/json", makeTelemetryJson(_telemetry));
}

void HttpServer::handleNotFound() {
    String msg = "{\"error\":\"Not found\",\"path\":\"" + _server.uri() + "\"}";
    _server.send(404, "application/json", msg);
}

bool HttpServer::tryReadFloatArg(WebServer &s, const String &name, float &out) {
    if (!s.hasArg(name)) return false;
    String v = s.arg(name);
    v.trim();
    if (v.isEmpty()) return false;
    out = v.toFloat();
    return true;
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

    // read until separator
    int j = i;
    while (j < (int) json.length()) {
        char c = json[j];
        if (c == ',' || c == '}' || isspace((unsigned char) c)) break;
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
    // Keep it simple and predictable
    String s = "{";
    s += "\"hasData\":" + String(t.hasData ? "true" : "false");
    s += ",\"temperature\":" + (isnan(t.temperature) ? String("null") : String(t.temperature, 2));
    s += ",\"humidity\":" + (isnan(t.humidity) ? String("null") : String(t.humidity, 2));
    s += ",\"counter\":" + String(t.counter);
    s += ",\"lastUpdateMs\":" + String(t.lastUpdateMs);
    s += "}";
    return s;
}
