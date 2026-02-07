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

class UbidotsClient {
public:
    struct Config {
        const char *token = nullptr;
        const char *deviceLabel = nullptr;

        // Ubidots Industrial MQTT broker
        const char *host = "industrial.api.ubidots.com";
        uint16_t port = 1883;

        const char *clientId = "gateway-arduino";
        uint32_t reconnectIntervalMs = 3000;
    };

    explicit UbidotsClient(const Config &cfg);

    void begin();
    void update(); // call in loop()

    // PubSubClient::connected() is NOT const, so this can't be const either.
    bool isConnected();

    // Publishes { "temperature": x, "humidity": y } to /v1.6/devices/<deviceLabel>
    bool publishTelemetry(float temperature, float humidity);

private:
    Config _cfg;
    WiFiClient _net;
    PubSubClient _mqtt;

    uint32_t _lastReconnectAttempt = 0;

    bool ensureConnected();

    String makeTopic() const;
    String makePayload(float temperature, float humidity) const;
};

#endif // GATEWAY_ARDUINO_UBIDOTSCLIENT_H
