/**
 * @file HttpServer.h
 * @brief HttpServer.h API.
 */

//
// Created by Josemar Carvalho on 07/02/26.
//

#ifndef GATEWAY_ARDUINO_HTTPSERVER_H
#define GATEWAY_ARDUINO_HTTPSERVER_H

#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <functional>

// SecureHttp (gateway side)
#include <SecureGatewayAuth.h>

/**
 * @brief class HttpServer.
 */
class HttpServer {
public:
    /**
     * @brief struct Telemetry.
     */
    struct Telemetry {
        bool hasData = false;

        float temperature = NAN;
        float humidity = NAN;

        // NOVOS CAMPOS (payload novo)
        int fuelLevel = -1; // <0 = ausente
        float stepperSpeed = NAN; // NAN = ausente
        float stepperRpm = NAN; // NAN = ausente

        uint32_t counter = 0;
        uint32_t lastUpdateMs = 0;
    };

    using TelemetryCallback = std::function<void(const Telemetry &)>;

    /**
     * @brief HttpServer.
     */
    explicit HttpServer(uint16_t port);

    /**
     * @brief begin.
     */
    void begin();

    /**
     * @brief update.
     */
    void update();

    const Telemetry &telemetry() const;

    // Ubidots (imediato)
    /**
     * @brief onTelemetryUpdated.
     */
    void onTelemetryUpdated(TelemetryCallback cb);

    // ThingSpeak (a cada 30s)
    /**
     * @brief onThingSpeakDue.
     */
    void onThingSpeakDue(TelemetryCallback cb);

    /**
     * @brief setThingSpeakIntervalMs.
     */
    void setThingSpeakIntervalMs(uint32_t intervalMs);

    // Gate interno para evitar chamar ThingSpeak fora do tempo
    bool canSendThingSpeakNow() const;

    /**
     * @brief markThingSpeakSent.
     */
    void markThingSpeakSent();

private:
    void registerRoutes();

    /**
     * @brief handleRoot.
     */
    void handleRoot();

    /**
     * @brief handleTelemetryGet.
     */
    void handleTelemetryGet();

    /**
     * @brief handleTelemetryPost.
     */
    void handleTelemetryPost();

    /**
     * @brief handleNotFound.
     */
    void handleNotFound();

    /**
     * @brief tryExtractJsonNumber.
     */
    bool tryExtractJsonNumber(const String &json, const char *key, float &out);

    /**
     * @brief makeTelemetryJson.
     */
    String makeTelemetryJson(const Telemetry &t);

    /**
     * @brief tickThingSpeakTimer.
     */
    void tickThingSpeakTimer();

    // ====== Security helpers ======
    void setupSecureHeadersCollection();

    // ❗removido const (WebServer::client() não é const)
    bool isClientAllowed();

private:
    WebServer _server;

    // SecureHttp verifier/decryptor (uses SecureHttpConfig.h)
    SecureGatewayAuth _secureAuth;

    Telemetry _telemetry;

    TelemetryCallback _onTelemetryUpdated; // Ubidots
    TelemetryCallback _onThingSpeakDue; // ThingSpeak

    uint32_t _thingSpeakIntervalMs = 30000; // 30s
    uint32_t _lastThingSpeakSendMs = 0; // último envio
    bool _thingSpeakPending = false; // dado pendente
};

#endif // GATEWAY_ARDUINO_HTTPSERVER_H
