#include <Arduino.h>
#include <WiFi.h>

#include "secrets.h"
#include <WiFiManager.h>
#include <LedStatus.h>

#include <DhtSensor.h>
#include <GatewayClient.h>
#include <FuelLevel.h>

#include <AccelStepper.h>

#define LED_PIN 2
#define DHT_PIN 4

// Pot (ADC1 recomendado)
#define FUEL_ADC_PIN 34

// Pot de velocidade do stepper (ADC1 recomendado)
#define SPEED_ADC_PIN 35

#define GATEWAY_HOST "192.168.3.12"
#define GATEWAY_PORT 8045

// ULN2003 -> ESP32 GPIOs (sugestão segura)
#define STEPPER_IN1 14
#define STEPPER_IN2 27
#define STEPPER_IN3 26
#define STEPPER_IN4 25

// Intervalos (ms)
static const uint32_t PRINT_INTERVAL_MS = 5000;
static const uint32_t SEND_INTERVAL_MS = 15000; // teste visual: reduz travas do HTTP
static const uint32_t STEPPER_UPDATE_MS = 100; // atualiza speed do stepper pelo pot

// Velocidade (steps/s) - TESTE VISUAL (pra ver LEDs mudando)
static const float STEPPER_MIN_SPEED = 5.0f; // bem lento (visível)
static const float STEPPER_MAX_SPEED = 80.0f; // ainda visível

// Half-step recomendado pro 28BYJ-48
AccelStepper stepper(AccelStepper::HALF4WIRE, STEPPER_IN1, STEPPER_IN3, STEPPER_IN2, STEPPER_IN4);

LedStatus led(LED_PIN);
WiFiManager *wifi = nullptr;
DhtSensor *dht = nullptr;
GatewayClient *gateway = nullptr;
FuelLevel *fuel = nullptr;

static bool lastWifiConnected = false;
static uint32_t lastPrintMs = 0;
static uint32_t lastSendAttemptMs = 0;
static uint32_t lastStepperUpdateMs = 0;

static void logGatewayFail() {
    Serial.print("[Gateway] Send failed err=");
    Serial.print((int) gateway->lastError());
    Serial.print(" http=");
    Serial.println(gateway->lastHttpStatus());
}

// Leitura rápida do ADC (sem delay) para não "engasgar" o stepper
static int readAdcFast(uint8_t pin, uint8_t samples = 3) {
    if (samples == 0) samples = 1;
    long sum = 0;
    for (uint8_t i = 0; i < samples; i++) {
        sum += analogRead(pin);
    }
    return (int) (sum / samples);
}

static float speedFromPot(int adcRaw) {
    adcRaw = constrain(adcRaw, 0, 4095);
    const float t = (float) adcRaw / 4095.0f;
    return STEPPER_MIN_SPEED + t * (STEPPER_MAX_SPEED - STEPPER_MIN_SPEED);
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
    dht->begin();

    // Fuel (potenciômetro -> 0..100%)
    FuelLevel::Config fcfg;
    fcfg.adcPin = FUEL_ADC_PIN;
    fcfg.adcMin = 200; // ajuste com base no raw real
    fcfg.adcMax = 3800; // ajuste com base no raw real
    fcfg.invert = false;
    fcfg.samples = 10;

    fuel = new FuelLevel(fcfg);
    fuel->begin();

    // Stepper + pot de velocidade
    pinMode(SPEED_ADC_PIN, INPUT);

    // ADC do ESP32
    analogReadResolution(12); // 0..4095

    stepper.setMaxSpeed(STEPPER_MAX_SPEED);
    stepper.setSpeed(STEPPER_MIN_SPEED);

    // Gateway client
    GatewayClient::Config gcfg;
    gcfg.host = GATEWAY_HOST;
    gcfg.port = GATEWAY_PORT;
    gcfg.path = "/telemetry";
    gcfg.minIntervalMs = SEND_INTERVAL_MS; // rate-limit interno
    gcfg.timeoutMs = 800; // reduz congelamentos no stepper

    gateway = new GatewayClient(gcfg);
    gateway->setDebugStream(&Serial);
    gateway->begin();

    Serial.print("[Device] MAC: ");
    Serial.println(WiFi.macAddress());

    Serial.println("[Fuel] Calibration tip: observe raw at min/max and update adcMin/adcMax.");
    Serial.println("[Stepper] VISUAL TEST: speed 5..80 steps/s (LEDs should clearly change).");
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
            led.setMode(LedStatus::Mode::BLINK_FAST);
        }
    }

    if (dht) dht->update();

    const uint32_t now = millis();

    // ===== Stepper: atualiza speed pelo potenciômetro (não-bloqueante) =====
    static int speedRaw = 0;
    static float stepperSpeed = STEPPER_MIN_SPEED;

    if (now - lastStepperUpdateMs >= STEPPER_UPDATE_MS) {
        lastStepperUpdateMs = now;

        speedRaw = readAdcFast(SPEED_ADC_PIN, 3);
        stepperSpeed = speedFromPot(speedRaw);
        stepper.setSpeed(stepperSpeed);
    }

    // roda sempre para não perder steps
    stepper.runSpeed();

    // ===== Fuel =====
    const int fuelRaw = (fuel) ? fuel->readRaw() : -1;
    const int fuelPct = (fuel) ? fuel->readPercent() : -1;

    // ===== Print local a cada 5s =====
    if (now - lastPrintMs >= PRINT_INTERVAL_MS) {
        lastPrintMs = now;

        if (dht && dht->hasData()) {
            const auto &r = dht->data();
            Serial.printf("[DHT] T=%.2f C | H=%.2f %%\n", r.temperature, r.humidity);
        } else {
            Serial.println("[DHT] no valid data yet");
        }

        Serial.printf("[Fuel] raw=%d | level=%d %%\n", fuelRaw, fuelPct);

        const float stepPeriodMs = (stepperSpeed > 0.0f) ? (1000.0f / stepperSpeed) : -1.0f;
        Serial.printf("[Stepper] raw=%d | speed=%.1f steps/s | period=%.1f ms/step\n",
                      speedRaw, stepperSpeed, stepPeriodMs);
    }

    // ===== Envio pro gateway =====
    if (connectedNow && dht && dht->hasData() && gateway && fuel) {
        if (now - lastSendAttemptMs >= SEND_INTERVAL_MS) {
            lastSendAttemptMs = now;

            const auto &r = dht->data();

            const bool ok = gateway->publishTelemetry(r.temperature, r.humidity, fuelPct, stepperSpeed);

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
