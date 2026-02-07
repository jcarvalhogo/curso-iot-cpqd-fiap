#include <Arduino.h>

#include "secrets.h"
#include "LedStatus.h"
#include "WiFiManager.h"

#define LED_PIN 2

LedStatus led(LED_PIN);

WiFiManager *wifi = nullptr;

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("\nBoot gateway-arduino");

    led.begin();
    led.setMode(LedStatus::Mode::BLINK_FAST);

    WiFiManager::Config cfg;
    cfg.ssid = WIFI_SSID;
    cfg.pass = WIFI_PASS;
    cfg.hostname = "gateway-arduino";
    cfg.connectTimeoutMs = 20000;
    cfg.sleep = true;
    cfg.txPower = WIFI_POWER_8_5dBm;

    // Cria o WiFiManager com config já preenchida
    wifi = new WiFiManager(cfg);

    wifi->begin();
    wifi->connect();

    led.setMode(wifi->isConnected()
                    ? LedStatus::Mode::BLINK_SLOW
                    : LedStatus::Mode::OFF);
}

void loop() {
    if (wifi) {
        wifi->update();

        if (wifi->isConnected()) led.setMode(LedStatus::Mode::BLINK_SLOW);
        else led.setMode(LedStatus::Mode::OFF);
    }

    led.update();
}
