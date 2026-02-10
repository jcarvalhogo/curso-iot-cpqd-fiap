//
// Created by Josemar Carvalho on 08/02/26.
//

#ifndef GATEWAY_ARDUINO_THINGSPEAKCLIENT_H
#define GATEWAY_ARDUINO_THINGSPEAKCLIENT_H

#pragma once

#include <Arduino.h>
#include <HttpServer.h>

class ThingSpeakClient {
public:
    struct Config {
        const char *writeApiKey = nullptr;

        // Default ThingSpeak endpoint (HTTP)
        const char *host = "api.thingspeak.com";
        uint16_t port = 80;

        // Segurança contra rate limit (ThingSpeak costuma aceitar ~15s; aqui usamos 20s)
        uint32_t minIntervalMs = 20000;

        // Se true: permite publicar mesmo com apenas um campo presente.
        // Se false: exige pelo menos temperature e humidity válidos.
        bool allowPartialTelemetry = false;

        bool isValid() const {
            return writeApiKey != nullptr && writeApiKey[0] != '\0'
                   && host != nullptr && host[0] != '\0'
                   && port != 0;
        }
    };

    enum class Error : uint8_t {
        None = 0,
        WifiNotConnected,
        InvalidConfig,
        InvalidTelemetry,
        RateLimited,
        ConnectFailed,
        Timeout,
        HttpBadStatus,
        WriteFailed // entry_id <= 0
    };

    explicit ThingSpeakClient(const Config &cfg);

    void begin(); // reservado (mantém padrão)
    void update(); // reservado

    void setDebugStream(Stream *s);

    // Publica usando a telemetria do gateway.
    bool publishTelemetry(const HttpServer::Telemetry &t);

    // Publica direto (field1=temperature, field2=humidity).
    bool publish(float temperature, float humidity);

    // Publica direto (field1..field5).
    // Mapeamento:
    // field1=temperature, field2=humidity, field3=fuelLevel, field4=stepperSpeed, field5=stepperRpm
    // Opcionais:
    // - Use NAN para floats que deseja omitir
    // - Use fuelLevel < 0 para omitir fuelLevel
    bool publish(float temperature,
                 float humidity,
                 int fuelLevel,
                 float stepperSpeed,
                 float stepperRpm);

    // Diagnóstico
    Error lastError() const noexcept;

    int lastHttpStatus() const noexcept;

    long lastEntryId() const noexcept;

    uint32_t lastPublishMs() const noexcept;

    static String maskKey(const char *key);

private:
    Config _cfg{};
    Stream *_dbg = nullptr;

    Error _lastError = Error::None;
    int _lastHttpStatus = -1;
    long _lastEntryId = 0;
    uint32_t _lastPublishMs = 0;

    bool canPublishNow(uint32_t now) const;

    void dbgln(const String &s);

    bool telemetryIsPublishable(const HttpServer::Telemetry &t) const;
};

#endif // GATEWAY_ARDUINO_THINGSPEAKCLIENT_H
