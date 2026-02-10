#pragma once

#include <Arduino.h>

// SecureHttp (device side)
#include <SecureDeviceAuth.h>

class GatewayClient {
public:
    struct Config {
        const char *host = nullptr;
        uint16_t port = 8045;
        const char *path = "/telemetry";

        // SecureHttp
        const char *deviceId = "vehicle-device-01";

        uint32_t minIntervalMs = 2000; // evita flood
        uint32_t timeoutMs = 3000; // timeout de response
    };

    enum class Error : uint8_t {
        None = 0,
        InvalidConfig,
        WifiNotConnected,
        RateLimited,
        ConnectFailed,
        Timeout,
        BadHttpStatus,
        SecureBuildFailed
    };

    explicit GatewayClient(const Config &cfg);

    void begin(); // reservado
    void update(); // reservado

    void setDebugStream(Stream *s);

    // Compatível com versão anterior
    bool publishTelemetry(float temperature, float humidity);

    bool publishTelemetry(float temperature, float humidity, int fuelLevelPercent);

    bool publishTelemetry(float temperature, float humidity, int fuelLevelPercent, float stepperSpeed);

    bool publishTelemetry(float temperature, float humidity, int fuelLevelPercent, float stepperSpeed,
                          float stepperRpm);

    Error lastError() const noexcept { return _lastError; }
    int lastHttpStatus() const noexcept { return _lastHttpStatus; }
    uint32_t lastPublishMs() const noexcept { return _lastPublishMs; }

private:
    Config _cfg{};
    Stream *_dbg = nullptr;

    SecureDeviceAuth _secure;

    Error _lastError = Error::None;
    int _lastHttpStatus = -1;
    uint32_t _lastPublishMs = 0;

    bool canPublishNow(uint32_t now) const;

    void dbgln(const String &s);

    bool isConfigValid() const;

    bool sendSecurePost(const String &plaintextJson);
};
