#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

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

// -----------------------------
// NTP / time sync helpers
// -----------------------------
static bool isTimeSynced() {
    time_t now = time(nullptr);
    return now >= 1700000000;
}

static void printTimeNow() {
    time_t now = time(nullptr);
    if (now < 1700000000) {
        Serial.printf("[Time] NOT SYNCED (epoch=%lu)\n", (unsigned long) now);
        return;
    }
    struct tm t{};
    localtime_r(&now, &t);
    Serial.printf("[Time] %04d-%02d-%02d %02d:%02d:%02d (epoch=%lu)\n",
                  t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                  t.tm_hour, t.tm_min, t.tm_sec,
                  (unsigned long) now);
}

static void syncTimeNtp(uint32_t timeoutMs = 15000) {
    // Trabalhar em epoch (UTC) é suficiente pro SecureHttp.
    configTime(0, 0, "pool.ntp.org", "time.nist.gov", "a.st1.ntp.br");

    const uint32_t start = millis();
    while (!isTimeSynced() && (millis() - start) < timeoutMs) {
        delay(250);
    }

    if (isTimeSynced()) {
        Serial.println("[Time] Gateway NTP synced");
        printTimeNow();
    } else {
        Serial.println("[Time] WARNING: NTP not synced (SecureHttp may reject timestamps)");
        printTimeNow();
    }
}

static void printHttpUrl() {
    Serial.print("[HTTP] Open: http://");
    Serial.print(WiFi.localIP());
    Serial.print(":");
    Serial.print(HTTP_PORT);
    Serial.println("/");
}

static void logTelemetryShort(const HttpServer::Telemetry &t) {
    Serial.print("[TEL] T=");
    Serial.print(t.temperature, 2);
    Serial.print(" H=");
    Serial.print(t.humidity, 2);

    Serial.print(" fuel=");
    Serial.print(t.fuelLevel);

    Serial.print(" speed=");
    if (isnan(t.stepperSpeed)) Serial.print("null");
    else Serial.print(t.stepperSpeed, 1);

    Serial.print(" rpm=");
    if (isnan(t.stepperRpm)) Serial.print("null");
    else Serial.print(t.stepperRpm, 2);

    Serial.println();
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

    // >>> define o intervalo do ThingSpeak pelo HttpServer (30s)
    http.setThingSpeakIntervalMs(30000);

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
    tcfg.minIntervalMs = 20000; // safety (HttpServer controla 30s)

    thingspeak = new ThingSpeakClient(tcfg);
    thingspeak->setDebugStream(&Serial);
    thingspeak->begin();

    Serial.print("[ThingSpeak] Config OK key=");
    Serial.println(ThingSpeakClient::maskKey(THINGSPEAK_WRITE_KEY));

    // Se já conectou no boot, sincroniza o relógio agora
    if (wifi && wifi->isConnected()) {
        Serial.println("[WiFi] Connected (boot)");
        printHttpUrl();
        syncTimeNtp();
    }

    // ============================================================
    // 1) Ubidots: IMEDIATO quando chega POST /telemetry válido
    // ============================================================
    http.onTelemetryUpdated([](const HttpServer::Telemetry &t) {
        if (!t.hasData) return;

        logTelemetryShort(t);

        if (ubidots) {
            const bool ok = ubidots->publishTelemetry(
                t.temperature,
                t.humidity,
                t.fuelLevel, // int (-1 se ausente)
                t.stepperSpeed, // float (NAN se ausente)
                t.stepperRpm // float (NAN se ausente)
            );
            Serial.println(ok ? "[Ubidots] Telemetry sent" : "[Ubidots] Send failed");
        }

        // >>> NÃO enviar ThingSpeak aqui (evita rate-limit)
    });

    // ============================================================
    // 2) ThingSpeak: SOMENTE quando o timer de 30s liberar
    // ============================================================
    http.onThingSpeakDue([](const HttpServer::Telemetry &t) {
        if (!t.hasData) return;

        logTelemetryShort(t);

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

            // ✅ Importante: sincroniza epoch no gateway para validar SecureHttp
            syncTimeNtp();
        } else {
            Serial.println("[WiFi] Disconnected");
            led.setMode(LedStatus::Mode::OFF);
        }
    }

    // IMPORTANTE:
    // http.update() precisa rodar para processar requisições E para o timer do ThingSpeak.
    http.update();

    if (connectedNow) {
        if (ubidots) ubidots->update();
        if (thingspeak) thingspeak->update();
    }

    led.update();
}
