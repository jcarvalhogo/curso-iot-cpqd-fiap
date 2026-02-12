#include <Arduino.h>
#include <Wire.h>

#include "LedRgbStatus.h"
#include "OledSh1107.h"
#include "Dht22Sensor.h"
#include "Mq2GasSensor.h"

#include "RtcClock.h"
#include "UiDashboard.h"
#include "AppStateMachine.h"

// I2C
static constexpr uint8_t I2C_SDA   = 21;
static constexpr uint8_t I2C_SCL   = 22;
static constexpr uint8_t OLED_ADDR = 0x3C;

// Sensores
static constexpr uint8_t DHT_PIN = 4;
static constexpr uint8_t MQ2_PIN = 34;

// App params
static constexpr bool  MQ2_SIM_MODE = true;
static constexpr float SMOKE_WARN_PPM_ENTER = 300.0f;
static constexpr float SMOKE_WARN_PPM_EXIT  = 200.0f;

// Períodos
static constexpr uint32_t DHT_LOG_MS = 2000;
static constexpr uint32_t MQ2_LOG_MS = 800;

OledSh1107 oled(I2C_SDA, I2C_SCL, OLED_ADDR);
LedRgbStatus led;
Dht22Sensor dht(DHT_PIN, 2000);
Mq2GasSensor mq2(MQ2_PIN);

RtcClock rtc(500);
UiDashboard ui(oled, rtc, 800);
AppStateMachine fsm(led);

// runtime
static bool oledOk = false;
static bool mq2Calibrated = false;
static uint32_t dhtFailCount = 0;

static uint32_t lastHeartbeatMs = 0;
static uint32_t lastDhtLogMs = 0;
static uint32_t lastMq2LogMs = 0;

static Dht22Sensor::Reading lastDht{false, NAN, NAN, 0};
static Mq2GasSensor::Reading lastMq2{false, 0, 0.0f, 0.0f, 0};
static float lastSmokePpm = NAN;
static float lastLpgPpm   = NAN;

void setup() {
    Serial.begin(115200);
    delay(200);

    led.begin();

    Wire.begin(I2C_SDA, I2C_SCL);
    delay(10);

    oledOk = oled.begin();
    Serial.printf("[OLED] %s\n", oledOk ? "OK" : "FAIL");

    rtc.begin();

    dht.begin();
    mq2.begin();
    mq2.setVref(3.3f);
    mq2.setAdcMax(4095);
    mq2.setVcc(5.0f);
    mq2.setLoadResistanceKOhm(5.0f);
    mq2.setMaxVoltage(3.3f);

    fsm.setBoot();
    if (oledOk) oled.drawStatus3("Sensores", "Iniciando...", "Aguarde");

    delay(2000);

    if (!MQ2_SIM_MODE) {
        mq2Calibrated = mq2.calibrateCleanAir(60, 50, 9.83f);
        Serial.printf("[MQ2] calib=%d\n", mq2Calibrated ? 1 : 0);
    } else {
        mq2Calibrated = true;
    }
}

void loop() {
    led.update();
    rtc.poll();

    const uint32_t now = millis();

    if (now - lastHeartbeatMs > 1000) {
        lastHeartbeatMs = now;
        Serial.printf("[APP] loop alive | ms=%lu\n", (unsigned long)now);
    }

    // DHT
    if (dht.read()) {
        lastDht = dht.last();
        if (!lastDht.ok) dhtFailCount++;

        if (now - lastDhtLogMs >= DHT_LOG_MS) {
            lastDhtLogMs = now;
            Serial.printf("[DHT] ok=%d T=%.2fC H=%.2f%% err=%s\n",
                          lastDht.ok ? 1 : 0,
                          lastDht.temperatureC,
                          lastDht.humidity,
                          dht.lastError());
        }
    }

    // MQ2
    lastMq2 = mq2.read(15, 3);

    if (MQ2_SIM_MODE) {
        lastSmokePpm = mq2.ppmSimFromNormalized(lastMq2.normalized);
        lastLpgPpm   = lastSmokePpm;
    } else {
        const float rsro = mq2.rsRoRatioFromVoltage(lastMq2.voltage);
        lastSmokePpm = mq2.ppm(Mq2GasSensor::Gas::SMOKE, rsro);
        lastLpgPpm   = mq2.ppm(Mq2GasSensor::Gas::LPG, rsro);
    }

    if (now - lastMq2LogMs >= MQ2_LOG_MS) {
        lastMq2LogMs = now;
        Serial.printf("[MQ2] adc=%u v=%.3fV norm=%.3f smoke=%.1fppm\n",
                      (unsigned)lastMq2.adc, lastMq2.voltage, lastMq2.normalized, lastSmokePpm);
    }

    // FSM
    fsm.evaluate(MQ2_SIM_MODE, mq2Calibrated, lastDht.ok, lastSmokePpm,
                 SMOKE_WARN_PPM_ENTER, SMOKE_WARN_PPM_EXIT);

    // UI
    if (oledOk) {
        UiDashboard::Inputs in;
        in.mq2SimMode = MQ2_SIM_MODE;
        in.mq2Calibrated = mq2Calibrated;
        in.dht = lastDht;
        in.mq2 = lastMq2;
        in.smokePpm = lastSmokePpm;
        in.lpgPpm = lastLpgPpm;
        in.dhtFailCount = dhtFailCount;
        ui.tick(in);
    }
}
