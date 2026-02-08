#include <Arduino.h>
#include <WiFi.h>

#include "secrets.h"
#include <WiFiManager.h>
#include <LedStatus.h>
#include <DhtSensor.h>

#define LED_PIN 2
#define DHT_PIN 4

LedStatus led(LED_PIN);
WiFiManager *wifi = nullptr;
DhtSensor *dht = nullptr;

static bool lastWifiConnected = false;
static uint32_t lastPrintMs = 0;

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("\nBoot vehicle-device");

    led.begin();
    led.setMode(LedStatus::Mode::BLINK_FAST);

    WiFiManager::Config cfg;
    cfg.ssid = WIFI_SSID;
    cfg.pass = WIFI_PASS;
    cfg.hostname = "vehicle-device";
    cfg.connectTimeoutMs = 20000;
    cfg.sleep = true;
    cfg.txPower = WIFI_POWER_8_5dBm;

    wifi = new WiFiManager(cfg);
    wifi->begin();
    wifi->connect();
    lastWifiConnected = (wifi && wifi->isConnected());

    DhtSensor::Config dcfg;
    dcfg.pin = DHT_PIN;
    dcfg.type = DhtSensor::Type::DHT22; // troque p/ DHT11 se for o seu
    dcfg.minIntervalMs = 2000;

    dht = new DhtSensor(dcfg);
    dht->setDebugStream(&Serial);
    dht->begin();

    Serial.print("[Device] MAC: ");
    Serial.println(WiFi.macAddress());
}

void loop() {
    if (wifi) wifi->update();

    const bool connectedNow = (wifi && wifi->isConnected());
    if (connectedNow != lastWifiConnected) {
        lastWifiConnected = connectedNow;

        if (connectedNow) {
            Serial.println("[WiFi] Connected");
            wifi->printNetInfo(Serial);
            led.setMode(LedStatus::Mode::BLINK_SLOW);
        } else {
            Serial.println("[WiFi] Disconnected");
            led.setMode(LedStatus::Mode::OFF);
        }
    }

    if (dht) dht->update();

    const uint32_t now = millis();
    if (now - lastPrintMs >= 5000) {
        lastPrintMs = now;

        if (dht && dht->hasData()) {
            const auto &r = dht->data();
            Serial.printf("[DHT] T=%.2f C | H=%.2f %% | ms=%lu\n",
                          r.temperature, r.humidity, (unsigned long) r.lastReadMs);
        } else {
            Serial.println("[DHT] no valid data yet");
        }
    }

    led.update();
}
