#include <Arduino.h>
#include "LedRgbStatus.h"
#include "OledSh1107.h"
#include "Dht22Sensor.h"
#include "Mq2GasSensor.h"

// ESP32 clássico: I2C padrão (SDA=21, SCL=22)
static constexpr uint8_t I2C_SDA = 21;
static constexpr uint8_t I2C_SCL = 22;
static constexpr uint8_t OLED_ADDR = 0x3C;

// Sensores
static constexpr uint8_t DHT_PIN = 4; // DHT22
static constexpr uint8_t MQ2_PIN = 34; // MQ-2 AOUT (ADC1)

// Períodos
static constexpr uint32_t UI_REFRESH_MS = 800;

// Thresholds (PPM) — ajuste depois
static constexpr float SMOKE_WARN_PPM_ENTER = 300.0f;
static constexpr float SMOKE_WARN_PPM_EXIT = 200.0f;

// ✅ modo simulador: PPM varia de 0.1 a 100000 via normalized
static constexpr bool MQ2_SIM_MODE = true;

// Debug helpers
static uint32_t lastHeartbeatMs = 0;
static uint32_t lastReadLogMs = 0;
static bool firstReadDone = false;

OledSh1107 oled(I2C_SDA, I2C_SCL, OLED_ADDR);
LedRgbStatus led;
Dht22Sensor dht(DHT_PIN, 2000);
Mq2GasSensor mq2(MQ2_PIN);

enum class State : uint8_t { Boot, Running, SensorError, GasWarn, CalibFail };

static State st = State::Boot;
static bool oledOk = false;

static uint32_t failCount = 0;
static uint32_t lastUiMs = 0;

static Dht22Sensor::Reading lastDht{false, NAN, NAN, 0};
static Mq2GasSensor::Reading lastMq2{false, 0, 0.0f, 0.0f, 0};

static bool mq2Calibrated = false;
static float lastRsRo = NAN;
static float lastSmokePpm = NAN;
static float lastLpgPpm = NAN;

static void setState(State s) {
    if (st == s) return;
    st = s;

    switch (st) {
        case State::Boot: led.statusBoot();
            if (oledOk) oled.drawBootScreen("ESP32");
            break;
        case State::Running: led.statusOnline();
            break;
        case State::SensorError: led.statusError();
            break;
        case State::GasWarn: led.statusWarn();
            break;
        case State::CalibFail: led.statusError();
            break;
    }
}

static void formatPpm(char *out, size_t outSz, float ppm) {
    if (!isfinite(ppm)) {
        snprintf(out, outSz, "--");
        return;
    }

    if (ppm >= 100000.0f) snprintf(out, outSz, "100K");
    else if (ppm >= 1000.0f) snprintf(out, outSz, "%.1fK", ppm / 1000.0f);
    else if (ppm >= 100.0f) snprintf(out, outSz, "%.0f", ppm);
    else snprintf(out, outSz, "%.1f", ppm);
}

static void drawScreen() {
    if (!oledOk) return;

    char l1[32], l2[32], l3[32];
    const uint32_t up = millis() / 1000;

    snprintf(l1, sizeof(l1), "up %lus | MQ2 %s", (unsigned long) up,
             (MQ2_SIM_MODE ? "SIM" : (mq2Calibrated ? "R0 ok" : "calib!")));

    if (lastDht.ok) snprintf(l2, sizeof(l2), "T:%.1fC H:%.1f%%", lastDht.temperatureC, lastDht.humidity);
    else snprintf(l2, sizeof(l2), "DHT: ERRO (%lu)", (unsigned long) failCount);

    if (isfinite(lastSmokePpm)) {
        char ppmTxt[12];
        formatPpm(ppmTxt, sizeof(ppmTxt), lastSmokePpm);
        snprintf(l3, sizeof(l3), "Smoke:%s N:%.2f", ppmTxt, lastMq2.normalized);
    } else {
        snprintf(l3, sizeof(l3), "MQ2 N:%.2f A:%u", lastMq2.normalized, (unsigned) lastMq2.adc);
    }

    oled.drawStatus3(l1, l2, l3);
}

void setup() {
    Serial.begin(115200);
    delay(400);

    led.begin();

    oledOk = oled.begin();
    Serial.printf("[OLED] %s (SDA=%u SCL=%u addr=0x%02X)\n",
                  oledOk ? "OK" : "FAIL", I2C_SDA, I2C_SCL, OLED_ADDR);

    dht.begin();
    Serial.printf("[DHT] init OK (GPIO=%u)\n", (unsigned) DHT_PIN);

    mq2.begin();
    mq2.setVref(3.3f);
    mq2.setAdcMax(4095);
    mq2.setVcc(5.0f);
    mq2.setLoadResistanceKOhm(5.0f);
    mq2.setMaxVoltage(3.3f);

    Serial.printf("[MQ2] init OK (AOUT GPIO=%u/ADC1)\n", (unsigned) MQ2_PIN);

    setState(State::Boot);
    if (oledOk) oled.drawStatus3("Sensores", "Iniciando...", "Aguarde");

    Serial.println("[MQ2] warm-up short (demo)...");
    delay(2000);

    if (!MQ2_SIM_MODE) {
        Serial.println("[MQ2] calibrating clean air (demo)...");
        mq2Calibrated = mq2.calibrateCleanAir(60, 50, 9.83f);
        if (mq2Calibrated) {
            Serial.printf("[MQ2] R0=%.2fkOhm\n", mq2.r0KOhm());
            setState(State::Running);
        } else {
            Serial.println("[MQ2] calibration FAIL (R0 not set)");
            setState(State::CalibFail);
        }
    } else {
        // no simulador, a gente não depende de R0
        mq2Calibrated = true;
        setState(State::Running);
    }

    lastUiMs = millis();
    drawScreen();
}

void loop() {
    led.update();
    const uint32_t now = millis();

    if (now - lastHeartbeatMs > 1000) {
        lastHeartbeatMs = now;
        Serial.printf("[APP] loop alive | ms=%lu\n", (unsigned long) now);
    }

    if (!firstReadDone && now > 2000) {
        firstReadDone = true;
        Serial.println("[SENS] first read attempt...");
    }

    const bool dhtUpdated = dht.read();
    if (!dhtUpdated && (now - lastReadLogMs > 2000)) {
        lastReadLogMs = now;
        Serial.println("[DHT] read() not updated yet (interval gating?)");
    }

    if (dhtUpdated) {
        lastDht = dht.last();
        const char *err = dht.lastError();

        if (!lastDht.ok) failCount++;

        Serial.printf("[DHT] ok=%d T=%.2fC H=%.2f%% err=%s\n",
                      lastDht.ok ? 1 : 0,
                      lastDht.temperatureC,
                      lastDht.humidity,
                      (err && err[0]) ? err : "-");

        if (!lastDht.ok) setState(State::SensorError);
    }

    // MQ2
    lastMq2 = mq2.read(15, 3);

    if (MQ2_SIM_MODE) {
        lastSmokePpm = mq2.ppmSimFromNormalized(lastMq2.normalized);
        lastLpgPpm = lastSmokePpm; // só pra demo

        Serial.printf("[MQ2] adc=%u v=%.3fV norm=%.3f smoke(sim)=%.1fppm\n",
                      (unsigned) lastMq2.adc, lastMq2.voltage, lastMq2.normalized, lastSmokePpm);
    } else {
        lastRsRo = mq2.rsRoRatioFromVoltage(lastMq2.voltage);
        lastSmokePpm = mq2.ppm(Mq2GasSensor::Gas::SMOKE, lastRsRo);
        lastLpgPpm = mq2.ppm(Mq2GasSensor::Gas::LPG, lastRsRo);

        Serial.printf("[MQ2] adc=%u v=%.3fV rs/ro=%.3f smoke=%.1fppm lpg=%.1fppm\n",
                      (unsigned) lastMq2.adc, lastMq2.voltage, lastRsRo, lastSmokePpm, lastLpgPpm);
    }

    // Estado por PPM (histerese)
    if (isfinite(lastSmokePpm) && lastSmokePpm > SMOKE_WARN_PPM_ENTER) {
        setState(State::GasWarn);
    } else if (isfinite(lastSmokePpm) && lastSmokePpm < SMOKE_WARN_PPM_EXIT) {
        if (lastDht.ok) setState(State::Running);
    }

    if (now - lastUiMs >= UI_REFRESH_MS) {
        lastUiMs = now;
        drawScreen();
    }
}
