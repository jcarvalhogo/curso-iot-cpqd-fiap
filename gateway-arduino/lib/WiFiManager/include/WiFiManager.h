//
// Created by Josemar Carvalho on 07/02/26.
//

#ifndef GATEWAY_ARDUINO_WIFIMANAGER_H
#define GATEWAY_ARDUINO_WIFIMANAGER_H

#pragma once
#include <Arduino.h>
#include <WiFi.h>

class WiFiManager {
public:
    struct Config {
        const char* ssid;
        const char* pass;
        const char* hostname = "gateway-arduino";
        uint32_t connectTimeoutMs = 20000;
        bool sleep = true;
        wifi_power_t txPower = WIFI_POWER_8_5dBm;
    };

    explicit WiFiManager(const Config& cfg);

    void begin();
    bool connect();          // tenta conectar (com timeout)
    void update();           // checa e tenta reconectar periodicamente

    bool isConnected() const;
    void printNetInfo(Stream& out) const;

private:
    Config _cfg;
    bool _connected = false;
    uint32_t _lastCheck = 0;

    static void onEvent(WiFiEvent_t event);
    static WiFiManager* _self;

    void handleEvent(WiFiEvent_t event);
};

#endif //GATEWAY_ARDUINO_WIFIMANAGER_H