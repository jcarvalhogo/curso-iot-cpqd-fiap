#include <Arduino.h>
#include <WiFi.h>
#include <math.h>

#include "secrets.h"
#include <WiFiManager.h>
#include <LedStatus.h>

#include <DhtSensor.h>
#include <GatewayClient.h>
#include <FuelLevel.h>

#include <AccelStepper.h>

#define LED_PIN 2
#define DHT_PIN 4

#define FUEL_ADC_PIN 34
#define SPEED_ADC_PIN 35

#define GATEWAY_HOST "192.168.3.12"
#define GATEWAY_PORT 8045

#define STEPPER_IN1 14
#define STEPPER_IN2 27
#define STEPPER_IN3 26
#define STEPPER_IN4 25

static const uint32_t PRINT_INTERVAL_MS = 5000;
static const uint32_t SEND_INTERVAL_MS =  5000; // teste visual
static const uint32_t STEPPER_UPDATE_MS = 80; // mais responsivo

// VISUAL (mais frenético)
static const float STEPPER_MIN_SPEED = 40.0f; // mínimo já "animado"
static const float STEPPER_MAX_SPEED = 600.0f; // máximo bem rápido (ajuste se perder passo)

// Curva de aceleração (sensação)
// > 1.0 => mais mudança no fim do curso do pot (efeito "acelera mais")
// < 1.0 => mais mudança no início
static const float SPEED_CURVE_GAMMA = 2.2f;

// RPM SIMULADO (telemetria)
static const float SIM_RPM_MIN = 800.0f;
static const float SIM_RPM_MAX = 8000.0f;

// Mesmo conceito de curva para RPM simulado (pode ser diferente, mas mantemos igual)
static const float RPM_CURVE_GAMMA = 2.2f;

AccelStepper stepper(AccelStepper::HALF4WIRE,
                     STEPPER_IN1, STEPPER_IN3, STEPPER_IN2, STEPPER_IN4);

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

static int readAdcFast(uint8_t pin, uint8_t samples = 3) {
    if (samples == 0) samples = 1;
    long sum = 0;
    for (uint8_t i = 0; i < samples; i++) sum += analogRead(pin);
    return (int) (sum / samples);
}

static float applyCurve01(float t, float gamma) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    // gamma > 1: acelera mais no final (sensação de "pisar")
    return powf(t, gamma);
}

static float speedFromPot(int adcRaw) {
    adcRaw = constrain(adcRaw, 0, 4095);
    float t = (float) adcRaw / 4095.0f;
    t = applyCurve01(t, SPEED_CURVE_GAMMA);
    return STEPPER_MIN_SPEED + t * (STEPPER_MAX_SPEED - STEPPER_MIN_SPEED);
}

static float simulatedRpmFromPot(int adcRaw) {
    adcRaw = constrain(adcRaw, 0, 4095);
    float t = (float) adcRaw / 4095.0f;
    t = applyCurve01(t, RPM_CURVE_GAMMA);
    return SIM_RPM_MIN + t * (SIM_RPM_MAX - SIM_RPM_MIN);
}

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("\nBoot vehicle-device");

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
    dcfg.type = DhtSensor::Type::DHT22;
    dcfg.minIntervalMs = 2000;

    dht = new DhtSensor(dcfg);
    dht->begin();

    // Fuel
    FuelLevel::Config fcfg;
    fcfg.adcPin = FUEL_ADC_PIN;
    fcfg.adcMin = 200;
    fcfg.adcMax = 3800;
    fcfg.invert = false;
    fcfg.samples = 10;

    fuel = new FuelLevel(fcfg);
    fuel->begin();

    // Stepper
    pinMode(SPEED_ADC_PIN, INPUT);
    analogReadResolution(12);

    stepper.setMaxSpeed(STEPPER_MAX_SPEED);
    stepper.setSpeed(STEPPER_MIN_SPEED);

    // Gateway
    GatewayClient::Config gcfg;
    gcfg.host = GATEWAY_HOST;
    gcfg.port = GATEWAY_PORT;
    gcfg.path = "/telemetry";
    gcfg.minIntervalMs = SEND_INTERVAL_MS;
    gcfg.timeoutMs = 800;

    gateway = new GatewayClient(gcfg);
    gateway->setDebugStream(&Serial);
    gateway->begin();

    Serial.print("[Device] MAC: ");
    Serial.println(WiFi.macAddress());

    Serial.println("[Fuel] Calibration tip: observe raw at min/max and update adcMin/adcMax.");
    Serial.printf("[Stepper] VISUAL: speed %.0f..%.0f steps/s | curve gamma=%.1f\n",
                  STEPPER_MIN_SPEED, STEPPER_MAX_SPEED, SPEED_CURVE_GAMMA);
    Serial.printf("[RPM] Simulated RPM: %.0f..%.0f | curve gamma=%.1f\n",
                  SIM_RPM_MIN, SIM_RPM_MAX, RPM_CURVE_GAMMA);
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

    // Stepper update
    static int speedRaw = 0;
    static float stepperSpeed = STEPPER_MIN_SPEED;
    static float stepperRpm = SIM_RPM_MIN;

    if (now - lastStepperUpdateMs >= STEPPER_UPDATE_MS) {
        lastStepperUpdateMs = now;

        speedRaw = readAdcFast(SPEED_ADC_PIN, 3);

        // Controle visual (stepper real) + curva de aceleração
        stepperSpeed = speedFromPot(speedRaw);
        stepper.setSpeed(stepperSpeed);

        // Telemetria (RPM simulado) + curva
        stepperRpm = simulatedRpmFromPot(speedRaw);
    }

    // roda sempre para não perder steps
    stepper.runSpeed();

    // Fuel
    const int fuelRaw = (fuel) ? fuel->readRaw() : -1;
    const int fuelPct = (fuel) ? fuel->readPercent() : -1;

    // Print
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
        Serial.printf("[Stepper] raw=%d | speed=%.1f steps/s | rpm(sim)=%.0f | period=%.2f ms/step\n",
                      speedRaw, stepperSpeed, stepperRpm, stepPeriodMs);
    }

    // Send telemetry
    if (connectedNow && dht && dht->hasData() && gateway && fuel) {
        if (now - lastSendAttemptMs >= SEND_INTERVAL_MS) {
            lastSendAttemptMs = now;

            const auto &r = dht->data();

            const bool ok = gateway->publishTelemetry(
                r.temperature, r.humidity,
                fuelPct,
                stepperSpeed,
                stepperRpm
            );

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
