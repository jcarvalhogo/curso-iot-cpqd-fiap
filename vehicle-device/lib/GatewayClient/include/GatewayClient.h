//
// Created by Josemar Carvalho on 08/02/26.
//

#ifndef VEHICLE_DEVICE_GATEWAYCLIENT_H
#define VEHICLE_DEVICE_GATEWAYCLIENT_H

#pragma once

#include <Arduino.h>

class GatewayClient {
public:
    struct Config {
        const char *host = nullptr; // ex: "192.168.3.12" ou "gateway-arduino.local"
        uint16_t port = 8045;
        const char *path = "/telemetry";

        uint32_t minIntervalMs = 2000; // evita flood (padrão: 2s)
        uint32_t timeoutMs = 3000; // timeout de socket/response
    };

    enum class Error : uint8_t {
        None = 0,
        InvalidConfig,
        WifiNotConnected,
        RateLimited,
        ConnectFailed,
        SendFailed,
        Timeout,
        BadHttpStatus
    };

    explicit GatewayClient(const Config &cfg);

    void begin(); // reservado
    void update(); // reservado

    void setDebugStream(Stream *s);

    // Compatível com versão anterior (sem fuelLevel/stepperSpeed)
    bool publishTelemetry(float temperature, float humidity);

    // Nova versão (com fuelLevel). Se fuelLevelPercent < 0, não envia o campo.
    bool publishTelemetry(float temperature, float humidity, int fuelLevelPercent);

    // Versão completa: fuelLevel opcional + stepperSpeed opcional (steps/s)
    // stepperSpeed < 0 => não envia o campo.
    bool publishTelemetry(float temperature, float humidity, int fuelLevelPercent, float stepperSpeed);

    // Diagnóstico
    Error lastError() const noexcept { return _lastError; }
    int lastHttpStatus() const noexcept { return _lastHttpStatus; }
    uint32_t lastPublishMs() const noexcept { return _lastPublishMs; }

private:
    Config _cfg{};
    Stream *_dbg = nullptr;

    Error _lastError = Error::None;
    int _lastHttpStatus = -1;
    uint32_t _lastPublishMs = 0;

    bool canPublishNow(uint32_t now) const;

    void dbgln(const String &s);

    bool isConfigValid() const;
};

#endif // VEHICLE_DEVICE_GATEWAYCLIENT_H
