#include <Arduino.h>
#include <WiFi.h>

#include "secrets.h"
#include <WiFiManager.h>
#include <LedStatus.h>

#include <DhtSensor.h>
#include <GatewayClient.h>
#include <FuelLevel.h>

#define LED_PIN 2
#define DHT_PIN 4

// Pot (ADC1 recomendado)
#define FUEL_ADC_PIN 34

#define GATEWAY_HOST "192.168.3.12"
#define GATEWAY_PORT 8045

// Intervalos (ms)
static const uint32_t PRINT_INTERVAL_MS = 5000;
static const uint32_t SEND_INTERVAL_MS = 5000; // envia pro gateway a cada 5s

LedStatus led(LED_PIN);
WiFiManager *wifi = nullptr;
DhtSensor *dht = nullptr;
GatewayClient *gateway = nullptr;
FuelLevel *fuel = nullptr;

static bool lastWifiConnected = false;
static uint32_t lastPrintMs = 0;
static uint32_t lastSendAttemptMs = 0;

static void logGatewayFail() {
    Serial.print("[Gateway] Send failed err=");
    Serial.print((int) gateway->lastError());
    Serial.print(" http=");
    Serial.println(gateway->lastHttpStatus());
}

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("\nBoot vehicle-device");

    // LED
    led.begin();
    led.setMode(LedStatus::Mode::BLINK_FAST);

    // Wi-Fi
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
    led.setMode(lastWifiConnected ? LedStatus::Mode::BLINK_SLOW : LedStatus::Mode::BLINK_FAST);

    // DHT
    DhtSensor::Config dcfg;
    dcfg.pin = DHT_PIN;
    dcfg.type = DhtSensor::Type::DHT22; // ajuste se for DHT11
    dcfg.minIntervalMs = 2000;

    dht = new DhtSensor(dcfg);
    // dht->setDebugStream(&Serial); // opcional
    dht->begin();

    // Fuel (potenciômetro -> 0..100%)
    FuelLevel::Config fcfg;
    fcfg.adcPin = FUEL_ADC_PIN;
    // Valores iniciais de calibração (ajuste depois com base no raw real)
    fcfg.adcMin = 200; // pot no "vazio"
    fcfg.adcMax = 3800; // pot no "cheio"
    fcfg.invert = false;
    fcfg.samples = 10;

    fuel = new FuelLevel(fcfg);
    fuel->begin();

    // Gateway client
    GatewayClient::Config gcfg;
    gcfg.host = GATEWAY_HOST;
    gcfg.port = GATEWAY_PORT;
    gcfg.path = "/telemetry";
    gcfg.minIntervalMs = SEND_INTERVAL_MS; // rate-limit interno
    gcfg.timeoutMs = 3000;

    gateway = new GatewayClient(gcfg);
    gateway->setDebugStream(&Serial);
    gateway->begin();

    Serial.print("[Device] MAC: ");
    Serial.println(WiFi.macAddress());

    Serial.println("[Fuel] Calibration tip: observe raw at min/max and update adcMin/adcMax.");
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
            // melhor pra debug do device: mostra que caiu rede
            led.setMode(LedStatus::Mode::BLINK_FAST);
        }
    }

    if (dht) dht->update();

    const uint32_t now = millis();

    // Leitura do fuel (faz uma vez por loop; barato o suficiente)
    const int fuelRaw = (fuel) ? fuel->readRaw() : -1;
    const int fuelPct = (fuel) ? fuel->readPercent() : -1;

    // Print local a cada 5s
    if (now - lastPrintMs >= PRINT_INTERVAL_MS) {
        lastPrintMs = now;

        if (dht && dht->hasData()) {
            const auto &r = dht->data();
            Serial.printf("[DHT] T=%.2f C | H=%.2f %%\n", r.temperature, r.humidity);
        } else {
            Serial.println("[DHT] no valid data yet");
        }

        Serial.printf("[Fuel] raw=%d | level=%d %%\n", fuelRaw, fuelPct);
    }

    // Envio pro gateway a cada SEND_INTERVAL_MS
    if (connectedNow && dht && dht->hasData() && gateway && fuel) {
        if (now - lastSendAttemptMs >= SEND_INTERVAL_MS) {
            lastSendAttemptMs = now;

            const auto &r = dht->data();

            const bool ok = gateway->publishTelemetry(r.temperature, r.humidity, fuelPct);
            if (ok) {
                Serial.println("[Gateway] Telemetry sent");
            } else {
                if (gateway->lastError() != GatewayClient::Error::RateLimited) {
                    logGatewayFail();
                }
            }
        }
    }

    led.update();
}
