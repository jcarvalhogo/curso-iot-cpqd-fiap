#include <Arduino.h>
#include "LedRgbStatus.h"
#include "OledSh1107.h"
#include "Dht22Sensor.h"

// ESP32 clássico: I2C padrão (SDA=21, SCL=22)
static constexpr uint8_t I2C_SDA = 21;
static constexpr uint8_t I2C_SCL = 22;
static constexpr uint8_t OLED_ADDR = 0x3C;

// DHT22 em GPIO 4
static constexpr uint8_t DHT_PIN = 4;

// Debug helpers
static uint32_t lastHeartbeatMs = 0;
static uint32_t lastReadLogMs = 0;
static bool firstReadDone = false;

OledSh1107 oled(I2C_SDA, I2C_SCL, OLED_ADDR);
LedRgbStatus led; // WS2812 externo no GPIO 5 (default na lib)
Dht22Sensor dht(DHT_PIN, 2000);

enum class State : uint8_t { Boot, Running, SensorError };

static State st = State::Boot;
static bool oledOk = false;
static uint32_t failCount = 0;

static void setState(State s) {
    if (st == s) return;
    st = s;

    switch (st) {
        case State::Boot:
            led.statusBoot();
            if (oledOk) oled.drawBootScreen("ESP32");
            break;
        case State::Running:
            led.statusOnline();
            break;
        case State::SensorError:
            led.statusError();
            break;
    }
}

static void drawReading(const Dht22Sensor::Reading &r, const char *err) {
    char l1[32], l2[32], l3[32];

    const uint32_t up = millis() / 1000;
    snprintf(l1, sizeof(l1), "DHT22 | up %lus", (unsigned long) up);

    if (r.ok) {
        snprintf(l2, sizeof(l2), "T: %.1f C", r.temperatureC);
        snprintf(l3, sizeof(l3), "H: %.1f %%", r.humidity);
    } else {
        snprintf(l2, sizeof(l2), "ERRO DHT (%lu)", (unsigned long) failCount);
        snprintf(l3, sizeof(l3), "%s", (err && err[0]) ? err : "TIMEOUT");
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

    setState(State::Boot);
    if (oledOk) oled.drawStatus3("DHT22", "Iniciando...", "Aguarde");

    delay(1500); // warm-up DHT

    setState(State::Running);
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
        Serial.println("[DHT] first read attempt...");
    }

    const bool updated = dht.read();

    if (!updated && (now - lastReadLogMs > 2000)) {
        lastReadLogMs = now;
        Serial.println("[DHT] read() not updated yet (interval gating?)");
    }

    if (updated) {
        const auto r = dht.last();
        const char *err = dht.lastError();

        if (!r.ok) failCount++;

        Serial.printf("[DHT] ok=%d T=%.2fC H=%.2f%% err=%s\n",
                      r.ok ? 1 : 0,
                      r.temperatureC,
                      r.humidity,
                      (err && err[0]) ? err : "-");

        if (oledOk) drawReading(r, err);

        if (!r.ok) setState(State::SensorError);
        else setState(State::Running);
    }
}
