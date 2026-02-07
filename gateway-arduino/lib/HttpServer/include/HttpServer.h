//
// Created by Josemar Carvalho on 07/02/26.
//

#ifndef GATEWAY_ARDUINO_HTTPSERVER_H
#define GATEWAY_ARDUINO_HTTPSERVER_H

#pragma once
#include <Arduino.h>
#include <WebServer.h>

class HttpServer {
public:
    struct Telemetry {
        bool hasData = false;
        float temperature = NAN;
        float humidity = NAN;
        uint32_t counter = 0;
        uint32_t lastUpdateMs = 0;
    };

    explicit HttpServer(uint16_t port = 8045);

    void begin();

    void update();

    const Telemetry &telemetry() const;

private:
    WebServer _server;
    Telemetry _telemetry;

    void registerRoutes();

    void handleRoot();

    void handleTelemetryGet();

    void handleTelemetryPost();

    void handleNotFound();

    static bool tryReadFloatArg(WebServer &s, const String &name, float &out);

    static bool tryExtractJsonNumber(const String &json, const char *key, float &out);

    static String makeTelemetryJson(const Telemetry &t);
};

#endif //GATEWAY_ARDUINO_HTTPSERVER_H
