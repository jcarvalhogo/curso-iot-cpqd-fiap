//
// Created by Josemar Carvalho on 07/02/26.
//

#include "WiFiManager.h"

/**
 * @file WiFiManager.cpp
 * @brief Implementation of WiFiManager.
 */

WiFiManager *WiFiManager::_self = nullptr;

WiFiManager::WiFiManager(const Config &cfg) : _cfg(cfg) {
}

void WiFiManager::begin() {
    _self = this;

    WiFi.mode(WIFI_STA);
    WiFi.setHostname(_cfg.hostname);
    WiFi.setSleep(_cfg.sleep);
    WiFi.setTxPower(_cfg.txPower);

    // Registra callback global e redireciona para a instância
    WiFi.onEvent(WiFiManager::onEvent);
}

bool WiFiManager::connect() {
    Serial.printf("\nConectando no Wi-Fi: %s\n", _cfg.ssid);

    // Limpa estado anterior para evitar “sujeira”
    WiFi.disconnect(true);
    delay(100);

    WiFi.begin(_cfg.ssid, _cfg.pass);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < _cfg.connectTimeoutMs) {
        Serial.print(".");
        delay(500);
    }
    Serial.println();

    _connected = (WiFi.status() == WL_CONNECTED);

    if (_connected) {
        Serial.println("[WiFi] Conectado!");
        printNetInfo(Serial);
    } else {
        Serial.println("[WiFi] Falha ao conectar (timeout).");
        Serial.print("[WiFi] Status: ");
        Serial.println(WiFi.status());
    }

    return _connected;
}

void WiFiManager::update() {
    // Checagem a cada 5s (evita flood)
    uint32_t now = millis();
    if (now - _lastCheck < 5000) return;
    _lastCheck = now;

    if (WiFi.status() != WL_CONNECTED) {
        if (_connected) Serial.println("[WiFi] Caiu! Tentando reconectar...");
        _connected = false;
        WiFi.reconnect();
    }
}

bool WiFiManager::isConnected() const {
    return _connected && (WiFi.status() == WL_CONNECTED);
}

void WiFiManager::printNetInfo(Stream &out) const {
    out.print("IP: ");
    out.println(WiFi.localIP());
    out.print("Gateway: ");
    out.println(WiFi.gatewayIP());
    out.print("MAC: ");
    out.println(WiFi.macAddress());
    out.print("RSSI: ");
    out.print(WiFi.RSSI());
    out.println(" dBm");
}

void WiFiManager::onEvent(WiFiEvent_t event) {
    if (_self) _self->handleEvent(event);
}

void WiFiManager::handleEvent(WiFiEvent_t event) {
    switch (event) {
        case SYSTEM_EVENT_STA_CONNECTED:
            Serial.println("[WiFi] Conectado ao AP");
            break;

        case SYSTEM_EVENT_STA_GOT_IP:
            _connected = true;
            Serial.println("[WiFi] IP obtido");
            printNetInfo(Serial);
            break;

        case SYSTEM_EVENT_STA_DISCONNECTED:
            _connected = false;
            Serial.println("[WiFi] Desconectado");
            break;

        default:
            break;
    }
}
