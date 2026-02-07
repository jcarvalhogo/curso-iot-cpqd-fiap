#include <Arduino.h>

#include "secrets.h"
#include "LedStatus.h"
#include "WiFiManager.h"
#include "HttpServer.h"

#define LED_PIN 2
#define HTTP_PORT 8045

LedStatus led(LED_PIN);
WiFiManager *wifi = nullptr;
HttpServer http(HTTP_PORT);

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
    }

    // Always keep LED state machine running
    led.update();
}
