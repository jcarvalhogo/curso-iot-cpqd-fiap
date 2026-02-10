//
// Created by Josemar Carvalho on 07/02/26.
//

/**
 * @file WiFiManager.h
 * @brief Simple Wi-Fi STA connection manager for ESP32 (Arduino framework).
 *
 * The goal of this library is to provide a small and predictable API for
 * connecting an ESP32 to a Wi-Fi Access Point (STA mode) and keeping the
 * connection alive.
 *
 * Features:
 *  - Configurable SSID/password (WPA/WPA2/WPA3 handled by the WiFi stack)
 *  - Hostname configuration
 *  - Power-saving (WiFi sleep) and TX power configuration
 *  - Connect with timeout
 *  - Periodic reconnect attempts without blocking the main loop
 *  - WiFi event callback forwarding to the instance
 *
 * @note This implementation uses a single static instance pointer to forward
 * WiFi events. Only one WiFiManager instance is supported at a time.
 */

#ifndef SHARED_LIBS_WIFIMANAGER_H
#define SHARED_LIBS_WIFIMANAGER_H

#pragma once
#include <Arduino.h>
#include <WiFi.h>

/**
 * @brief Wi-Fi station connection manager.
 */
class WiFiManager {
public:
    /**
     * @brief Runtime configuration.
     */
    struct Config {
        /// Access Point SSID.
        const char* ssid;
        /// Access Point password.
        const char* pass;
        /// DHCP hostname announced by the station.
        const char* hostname = "gateway-arduino";
        /// Max time to wait in connect() before giving up.
        uint32_t connectTimeoutMs = 20000;
        /// Enable/disable WiFi sleep.
        bool sleep = true;
        /// Transmit power.
        wifi_power_t txPower = WIFI_POWER_8_5dBm;
    };

    /**
     * @brief Construct a WiFiManager using the given configuration.
     */
    explicit WiFiManager(const Config& cfg);

    /**
     * @brief Initialize WiFi in STA mode and register event callback.
     */
    void begin();

    /**
     * @brief Attempt to connect to the configured AP (blocking until timeout).
     * @return true if connected and got IP, false on timeout.
     */
    bool connect();          // tenta conectar (com timeout)

    /**
     * @brief Periodic maintenance call (should be called from loop()).
     */
    void update();           // checa e tenta reconectar periodicamente

    /**
     * @brief Returns true if currently connected and WiFi.status() is WL_CONNECTED.
     */
    bool isConnected() const;

    /**
     * @brief Print network information (IP, gateway, MAC, RSSI).
     */
    void printNetInfo(Stream& out) const;

private:
    Config _cfg;
    bool _connected = false;
    uint32_t _lastCheck = 0;

    static void onEvent(WiFiEvent_t event);
    static WiFiManager* _self;

    void handleEvent(WiFiEvent_t event);
};

#endif // SHARED_LIBS_WIFIMANAGER_H