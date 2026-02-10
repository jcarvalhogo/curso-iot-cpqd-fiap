/**
 * @file UbidotsClient.h
 * @brief UbidotsClient.h API.
 */

//
// Created by Josemar Carvalho on 07/02/26.
//

#ifndef GATEWAY_ARDUINO_UBIDOTSCLIENT_H
#define GATEWAY_ARDUINO_UBIDOTSCLIENT_H

#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

/**

 * @brief class UbidotsClient.

 */

class UbidotsClient {
public:
    /**
     * @brief struct Config.
     */
    struct Config {
        const char *token = nullptr;
        const char *deviceLabel = nullptr;

        // Ubidots Industrial MQTT broker
        const char *host = "industrial.api.ubidots.com";
        uint16_t port = 1883;

        const char *clientId = "gateway-arduino";
        uint32_t reconnectIntervalMs = 3000;
    };

    /**

     * @brief UbidotsClient.

     */

    explicit UbidotsClient(const Config &cfg);

    /**

     * @brief begin.

     */

    void begin();

    /**

     * @brief update.

     */

    void update(); // call in loop()

    // PubSubClient::connected() is NOT const, so this can't be const either.
    /**
     * @brief isConnected.
     */
    bool isConnected();

    // Compatibilidade (antigo): publica apenas temperature/humidity
    /**
     * @brief publishTelemetry.
     */
    bool publishTelemetry(float temperature, float humidity);

    // Novo payload:
    // {"temperature":28.30,"humidity":68.30,"fuelLevel":77,"stepperSpeed":40.0,"stepperRpm":800.00}
    //
    // Optional:
    // - Use NAN for floats you want to omit
    // - Use fuelLevel < 0 to omit fuelLevel
    /**
     * @brief publishTelemetry.
     */
    bool publishTelemetry(float temperature,
                          float humidity,
                          int fuelLevel,
                          float stepperSpeed,
                          float stepperRpm);

private:
    Config _cfg;
    WiFiClient _net;
    PubSubClient _mqtt;

    uint32_t _lastReconnectAttempt = 0;

    /**

     * @brief ensureConnected.

     */

    bool ensureConnected();

    String makeTopic() const;

    String makePayload(float temperature,
                       float humidity,
                       int fuelLevel,
                       float stepperSpeed,
                       float stepperRpm) const;
};

#endif // GATEWAY_ARDUINO_UBIDOTSCLIENT_H
