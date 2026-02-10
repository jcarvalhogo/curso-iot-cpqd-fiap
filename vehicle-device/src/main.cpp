#include <Arduino.h>
#include <WiFi.h>
#include <math.h>
#include <time.h>

#include "secrets.h"
#include <WiFiManager.h>

#include <DhtSensor.h>
#include <GatewayClient.h>
#include <FuelLevel.h>

// Se você quiser usar SECURE_DEVICE_ID aqui, inclua o config do SecureHttp.
// (Só faça isso se o vehicle-device tiver acesso ao shared-libs/SecureHttp/include)
#include "SecureHttpConfig.h"

#define DHT_PIN 4

#define FUEL_ADC_PIN 34
#define SPEED_ADC_PIN 35   // potenciômetro para simular aceleração (ADC)

#define GATEWAY_HOST "192.168.3.12"
#define GATEWAY_PORT 8045

static const uint32_t PRINT_INTERVAL_MS = 5000;
static const uint32_t SEND_INTERVAL_MS = 5000;

static const float ACCEL_CURVE_GAMMA = 2.2f;

static const float ACCEL_MIN = 0.0f;
static const float ACCEL_MAX = 100.0f;

static const float SIM_RPM_MIN = 0.0f;
static const float SIM_RPM_MAX = 8000.0f;

static const uint8_t EMA_ALPHA_PCT = 15; // 0..100

WiFiManager *wifi = nullptr;
DhtSensor *dht = nullptr;
GatewayClient *gateway = nullptr;
FuelLevel *fuel = nullptr;

static bool lastWifiConnected = false;
static uint32_t lastPrintMs = 0;
static uint32_t lastSendAttemptMs = 0;

// -----------------------------
// NTP / time sync helpers
// -----------------------------
static bool isTimeSynced() {
    time_t now = time(nullptr);
    return now >= 1700000000;
}

static void syncTimeNtp(uint32_t timeoutMs = 15000) {
    // timezone: Brasil -03:00 (ajuste se quiser). Pode deixar 0 e trabalhar em epoch.
    // setenv("TZ", "BRT+3", 1); tzset();  // opcional

    // servidores padrão + pool
    configTime(0, 0, "pool.ntp.org", "time.nist.gov", "a.st1.ntp.br");

    const uint32_t start = millis();
    while (!isTimeSynced() && (millis() - start) < timeoutMs) {
        delay(250);
    }

    if (isTimeSynced()) {
        time_t now = time(nullptr);
        struct tm t{};
        localtime_r(&now, &t);
        Serial.printf("[Time] Synced: %04d-%02d-%02d %02d:%02d:%02d (epoch=%lu)\n",
                      t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                      t.tm_hour, t.tm_min, t.tm_sec,
                      (unsigned long) now);
    } else {
        Serial.println("[Time] WARNING: NTP not synced (SecureHttp will fail with time_not_synced)");
    }
}

// -----------------------------
// ADC helpers
// -----------------------------
static int readAdcFast(uint8_t pin, uint8_t samples = 5) {
    if (samples == 0) samples = 1;
    long sum = 0;
    for (uint8_t i = 0; i < samples; i++) sum += analogRead(pin);
    return (int) (sum / samples);
}

static float clamp01(float t) {
    if (t < 0.0f) return 0.0f;
    if (t > 1.0f) return 1.0f;
    return t;
}

static float applyCurve01(float t, float gamma) {
    t = clamp01(t);
    return powf(t, gamma);
}

static float adcToAccelPct(int adcRaw) {
    adcRaw = constrain(adcRaw, 0, 4095);
    float t = (float) adcRaw / 4095.0f;
    t = applyCurve01(t, ACCEL_CURVE_GAMMA);
    return ACCEL_MIN + t * (ACCEL_MAX - ACCEL_MIN);
}

static float accelToSimRpm(float accelPct) {
    float t = (accelPct - ACCEL_MIN) / (ACCEL_MAX - ACCEL_MIN);
    t = clamp01(t);
    return SIM_RPM_MIN + t * (SIM_RPM_MAX - SIM_RPM_MIN);
}

static void logGatewayFail() {
    Serial.print("[Gateway] Send failed err=");
    Serial.print((int) gateway->lastError());
    Serial.print(" http=");
    Serial.println(gateway->lastHttpStatus());
}

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("\nBoot vehicle-device (no stepper / no led)");

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

    if (lastWifiConnected) {
        Serial.println("[WiFi] Connected");
        wifi->printNetInfo(Serial);

        // ✅ NTP sync (necessário pro SecureHttp: SecureDeviceAuth exige epoch válido)
        syncTimeNtp();
    } else {
        Serial.println("[WiFi] Not connected at boot (time sync will be delayed)");
    }

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

    // Potenciômetro (aceleração)
    pinMode(SPEED_ADC_PIN, INPUT);
    analogReadResolution(12);

    // Opcional (melhora estabilidade em alguns setups)
    analogSetPinAttenuation(FUEL_ADC_PIN, ADC_11db);
    analogSetPinAttenuation(SPEED_ADC_PIN, ADC_11db);

    // Gateway
    GatewayClient::Config gcfg;
    gcfg.host = GATEWAY_HOST;
    gcfg.port = GATEWAY_PORT;
    gcfg.path = "/telemetry";
    gcfg.minIntervalMs = SEND_INTERVAL_MS;
    gcfg.timeoutMs = 800;

    gateway = new GatewayClient(gcfg);
    gateway->setDebugStream(&Serial);

    // Se seu GatewayClient tiver suporte a deviceId (não sei sua API),
    // o ideal é setar aqui para bater com SECURE_DEVICE_ID no gateway.
    // Exemplo (descomente se existir no seu client):
    // gateway->setDeviceId(SECURE_DEVICE_ID);

    gateway->begin();

    Serial.print("[Device] MAC: ");
    Serial.println(WiFi.macAddress());

    Serial.println("[Fuel] Calibration tip: observe raw at min/max and update adcMin/adcMax.");
    Serial.printf("[Accel] pot on GPIO%d | accel %.0f..%.0f %% | gamma=%.1f | EMA=%u%%\n",
                  SPEED_ADC_PIN, ACCEL_MIN, ACCEL_MAX, ACCEL_CURVE_GAMMA, EMA_ALPHA_PCT);
    Serial.printf("[RPM] simulated %.0f..%.0f\n", SIM_RPM_MIN, SIM_RPM_MAX);
}

void loop() {
    if (wifi) wifi->update();

    const bool connectedNow = (wifi && wifi->isConnected());
    if (connectedNow != lastWifiConnected) {
        lastWifiConnected = connectedNow;

        if (connectedNow) {
            Serial.println("[WiFi] Connected");
            wifi->printNetInfo(Serial);

            // ✅ assim que conectar, tenta sincronizar horário
            if (!isTimeSynced()) syncTimeNtp();
        } else {
            Serial.println("[WiFi] Disconnected");
        }
    }

    if (dht) dht->update();

    const uint32_t now = millis();

    // Fuel
    const int fuelRaw = (fuel) ? fuel->readRaw() : -1;
    const int fuelPct = (fuel) ? fuel->readPercent() : -1;

    // Accel
    static int accelRaw = 0;
    static float accelPct = 0.0f;
    static float simRpm = 0.0f;
    static bool accelInit = false;

    {
        accelRaw = readAdcFast(SPEED_ADC_PIN, 5);
        const float targetPct = adcToAccelPct(accelRaw);

        const float a = (float) EMA_ALPHA_PCT / 100.0f;
        accelPct = accelInit ? (a * targetPct + (1.0f - a) * accelPct) : targetPct;
        accelInit = true;

        simRpm = accelToSimRpm(accelPct);
    }

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
        Serial.printf("[Accel] raw=%d | accel=%.1f %% | rpm(sim)=%.0f\n", accelRaw, accelPct, simRpm);

        // Ajuda a diagnosticar SecureHttp
        if (!isTimeSynced()) {
            Serial.println("[Time] NOT SYNCED -> SecureHttp will fail (time_not_synced)");
        }
    }

    // Send telemetry
    if (connectedNow && dht && dht->hasData() && gateway && fuel) {
        if (now - lastSendAttemptMs >= SEND_INTERVAL_MS) {
            lastSendAttemptMs = now;

            const auto &r = dht->data();

            // Reaproveitando contrato atual do gateway:
            // stepperSpeed -> aceleração (%)
            // stepperRpm   -> rpm simulado
            const bool ok = gateway->publishTelemetry(
                r.temperature, r.humidity,
                fuelPct,
                accelPct,
                simRpm
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
}