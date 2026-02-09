//
// Created by Josemar Carvalho on 07/02/26.
//

#ifndef GATEWAY_ARDUINO_HTTPSERVER_H
#define GATEWAY_ARDUINO_HTTPSERVER_H

#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <functional>

class HttpServer {
public:
    struct Telemetry {
        bool hasData = false;
        float temperature = NAN;
        float humidity = NAN;
        uint32_t counter = 0;
        uint32_t lastUpdateMs = 0;
    };

    using TelemetryCallback = std::function<void(const Telemetry &)>;

    explicit HttpServer(uint16_t port);

    void begin();

    void update();

    const Telemetry &telemetry() const;

    // Ubidots (imediato)
    void onTelemetryUpdated(TelemetryCallback cb);

    // ThingSpeak (a cada 30s)
    void onThingSpeakDue(TelemetryCallback cb);

    void setThingSpeakIntervalMs(uint32_t intervalMs);

    // Gate interno para evitar chamar ThingSpeak fora do tempo
    bool canSendThingSpeakNow() const;

    void markThingSpeakSent();

private:
    void registerRoutes();

    void handleRoot();

    void handleTelemetryGet();

    void handleTelemetryPost();

    void handleNotFound();

    bool tryReadFloatArg(WebServer &s, const String &name, float &out);

    bool tryExtractJsonNumber(const String &json, const char *key, float &out);

    String makeTelemetryJson(const Telemetry &t);

    void tickThingSpeakTimer();

private:
    WebServer _server;
    Telemetry _telemetry;

    TelemetryCallback _onTelemetryUpdated; // Ubidots
    TelemetryCallback _onThingSpeakDue; // ThingSpeak

    uint32_t _thingSpeakIntervalMs = 30000; // 30s
    uint32_t _lastThingSpeakSendMs = 0; // último envio
    bool _thingSpeakPending = false; // dado pendente
};

#endif // GATEWAY_ARDUINO_HTTPSERVER_H
