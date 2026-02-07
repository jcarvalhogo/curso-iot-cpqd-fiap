#include <Arduino.h>
#include <WiFi.h>

#include "secrets.h"
#include "LedStatus.h"
#include "WiFiManager.h"
#include "HttpServer.h"
#include "UbidotsClient.h"

#define LED_PIN 2
#define HTTP_PORT 8045

LedStatus led(LED_PIN);
WiFiManager *wifi = nullptr;
HttpServer http(HTTP_PORT);

// Ubidots
UbidotsClient *ubidots = nullptr;

static bool lastWifiConnected = false;

static void printHttpUrl() {
    Serial.print("[HTTP] Open: http://");
    Serial.print(WiFi.localIP());
    Serial.print(":");
    Serial.print(HTTP_PORT);
    Serial.println("/");
}

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("\nBoot gateway-arduino");

    // LED status init
    led.begin();
    led.setMode(LedStatus::Mode::BLINK_FAST); // boot / connecting

    // WiFi manager config
    WiFiManager::Config cfg;
    cfg.ssid = WIFI_SSID;
    cfg.pass = WIFI_PASS;
    cfg.hostname = "gateway-arduino";
    cfg.connectTimeoutMs = 20000;
    cfg.sleep = true;
    cfg.txPower = WIFI_POWER_8_5dBm;

    wifi = new WiFiManager(cfg);
    wifi->begin();
    wifi->connect();

    // Start HTTP server regardless of Wi-Fi state.
    // We'll only call http.update() while connected.
    http.begin();

    // Ubidots client config
    UbidotsClient::Config ucfg;
    ucfg.token = UBIDOTS_TOKEN;
    ucfg.deviceLabel = UBIDOTS_DEVICE_LABEL;
    ucfg.clientId = "gateway-arduino";

    ubidots = new UbidotsClient(ucfg);
    ubidots->begin();

    // When POST /telemetry is accepted, publish to Ubidots
    http.onTelemetryUpdated([](const HttpServer::Telemetry &t) {
        if (!t.hasData) return;
        if (!ubidots) return;

        const bool ok = ubidots->publishTelemetry(t.temperature, t.humidity);
        Serial.println(ok ? "[Ubidots] Telemetry sent" : "[Ubidots] Send failed");
    });
}

void loop() {
    if (wifi) {
        wifi->update();
    }

    const bool connectedNow = (wifi && wifi->isConnected());

    // Only react when connection state changes (avoids repeated logs / repeated LED set)
    if (connectedNow != lastWifiConnected) {
        lastWifiConnected = connectedNow;

        if (connectedNow) {
            Serial.println("[WiFi] Connected");
            printHttpUrl();
            led.setMode(LedStatus::Mode::BLINK_SLOW);
        } else {
            Serial.println("[WiFi] Disconnected");
            led.setMode(LedStatus::Mode::OFF);
        }
    }

    // Serve HTTP only when Wi-Fi is up (so requests don't hang)
    if (connectedNow) {
        http.update();
        if (ubidots) ubidots->update(); // keep MQTT alive / reconnect if needed
    }

    // Always keep LED state machine running
    led.update();
}
