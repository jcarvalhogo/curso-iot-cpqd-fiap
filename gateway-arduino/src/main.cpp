#include <Arduino.h>
#include <WiFi.h>

#include "secrets.h"

// Forçando includes de libs do /lib (forma mais estável no PlatformIO)
#include "LedStatus.h"
#include "WiFiManager.h"
#include "HttpServer.h"
#include "UbidotsClient.h"
#include "ThingSpeakClient.h"

#define LED_PIN 2
#define HTTP_PORT 8045

LedStatus led(LED_PIN);
WiFiManager *wifi = nullptr;
HttpServer http(HTTP_PORT);

// Ubidots
UbidotsClient *ubidots = nullptr;

// ThingSpeak
ThingSpeakClient *thingspeak = nullptr;

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
    http.begin();

    // Ubidots client config
    UbidotsClient::Config ucfg;
    ucfg.token = UBIDOTS_TOKEN;
    ucfg.deviceLabel = UBIDOTS_DEVICE_LABEL;
    ucfg.clientId = "gateway-arduino";

    ubidots = new UbidotsClient(ucfg);
    ubidots->begin();

    // ThingSpeak client config
    ThingSpeakClient::Config tcfg;
    tcfg.writeApiKey = THINGSPEAK_WRITE_KEY; // coloque no secrets.h
    tcfg.minIntervalMs = 20000;              // 20s (seguro contra rate limit)

    thingspeak = new ThingSpeakClient(tcfg);
    thingspeak->setDebugStream(&Serial);
    thingspeak->begin();

    Serial.print("[ThingSpeak] Config OK key=");
    Serial.println(ThingSpeakClient::maskKey(THINGSPEAK_WRITE_KEY));

    // When POST /telemetry is accepted, publish to Ubidots + ThingSpeak
    http.onTelemetryUpdated([](const HttpServer::Telemetry &t) {
        if (!t.hasData) return;

        if (ubidots) {
            const bool ok = ubidots->publishTelemetry(t.temperature, t.humidity);
            Serial.println(ok ? "[Ubidots] Telemetry sent" : "[Ubidots] Send failed");
        }

        if (thingspeak) {
            const bool ok = thingspeak->publishTelemetry(t);
            Serial.println(ok ? "[ThingSpeak] Telemetry sent" : "[ThingSpeak] Send failed");

            if (!ok) {
                Serial.print("[ThingSpeak] err=");
                Serial.print((int) thingspeak->lastError());
                Serial.print(" http=");
                Serial.print(thingspeak->lastHttpStatus());
                Serial.print(" entry_id=");
                Serial.println(thingspeak->lastEntryId());
            }
        }
    });
}

void loop() {
    if (wifi) wifi->update();

    const bool connectedNow = (wifi && wifi->isConnected());

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

    if (connectedNow) {
        http.update();
        if (ubidots) ubidots->update();
        if (thingspeak) thingspeak->update();
    }

    led.update();
}