# Gateway Arduino – ESP32 HTTP → Ubidots (Technical Documentation)

## 1. Overview

This project implements an **IoT Gateway running on ESP32**, built with **Arduino Framework + PlatformIO**.
The gateway exposes an **HTTP REST API** for telemetry ingestion and forwards the received data to **Ubidots Industrial** using **MQTT**.

The design follows embedded best practices:
- Non-blocking main loop
- Explicit state machines
- Modular libraries
- Clear separation between transport, application logic, and hardware feedback

---

## 2. System Responsibilities

The gateway is responsible for:

- Managing Wi-Fi connectivity and reconnection
- Exposing a REST endpoint for telemetry ingestion
- Normalizing and validating telemetry payloads
- Publishing telemetry to Ubidots via MQTT
- Providing visual feedback via LED state machine

This device **does not** read sensors directly.  
It acts as an **integration bridge** between edge systems and the cloud.

---

## 3. Software Architecture

### 3.1 High-Level Architecture

```
main.cpp
 ├── LedStatus        (GPIO / Visual Feedback)
 ├── WiFiManager      (Connectivity + Reconnection)
 ├── HttpServer       (REST API + Telemetry parsing)
 └── UbidotsClient    (MQTT Publisher)
```

### 3.2 Design Principles

- **Single Responsibility**: Each library has one clear role
- **Pull-based loop**: All services are updated from `loop()`
- **No dynamic blocking**: No delays in communication paths
- **Edge-friendly**: Safe reconnection and failure handling

---

## 4. Directory Structure

```
gateway-arduino/
├── src/
│   └── main.cpp
│
├── lib/
│   ├── LedStatus/
│   ├── WiFiManager/
│   ├── HttpServer/
│   └── UbidotsClient/
│
├── include/
├── test/
├── secrets.h
├── platformio.ini
├── wokwi.toml
└── README.md
```

Each folder inside `lib/` is a **local PlatformIO library** with:
- `include/` → public interface
- `src/` → implementation

---

## 5. Wi-Fi Subsystem (WiFiManager)

### Responsibilities

- Configure STA mode
- Connect using credentials
- Detect connection loss
- Retry connection with backoff
- Avoid blocking the main loop

### Key API

```cpp
wifi->begin();
wifi->connect();
wifi->update();
wifi->isConnected();
```

All reconnection logic is encapsulated.

---

## 6. LED State Machine (LedStatus)

### Purpose

Provide **hardware-level diagnostics** without Serial Monitor.

### States

| State              | Meaning                    |
|--------------------|----------------------------|
| BLINK_FAST         | Boot / Wi-Fi connecting    |
| BLINK_SLOW         | Wi-Fi connected            |
| OFF                | Disconnected / error       |

### Implementation

- Timer-based (millis)
- No blocking delays
- Updated every loop cycle

---

## 7. HTTP Server (HttpServer)

### Transport

- Arduino `WebServer`
- TCP/IP over Wi-Fi
- Port: **8045**

### Endpoints

#### `GET /`

Health / discovery endpoint.

#### `GET /telemetry`

Returns last known telemetry snapshot.

#### `POST /telemetry`

Accepts telemetry data.

Supported payloads:
- JSON body
- Query parameters
- Form-urlencoded

### Telemetry Model

```cpp
struct Telemetry {
    bool hasData;
    float temperature;
    float humidity;
    uint32_t counter;
    uint32_t lastUpdateMs;
};
```

### Internal Flow

1. Parse payload
2. Validate fields
3. Update telemetry struct
4. Increment counter
5. Trigger callback (integration hook)

---

## 8. Cloud Integration (UbidotsClient)

### Protocol

- MQTT
- PubSubClient

### Broker

```
industrial.api.ubidots.com:1883
```

### Authentication

- Username = TOKEN
- Password = TOKEN

### Topic Format

```
/v1.6/devices/<device_label>
```

### Payload Format

```json
{
  "temperature": 22.50,
  "humidity": 55.00
}
```

### Reliability Strategy

- Lazy connection
- Automatic reconnect
- Loop-based keepalive
- Publish only when connected

---

## 9. Main Loop Execution Model

```cpp
loop() {
  wifi.update();
  http.update();
  ubidots.update();
  led.update();
}
```

No blocking calls, no delays.

---

## 10. Environment Handling

### Current Status

- Real hardware: supported
- Wokwi: configuration present, Wi-Fi not functional

### Planned Strategy

- Compile-time environment selection
- Mock Wi-Fi and MQTT layers
- Simulated telemetry sources

---

## 11. Build & Toolchain

- PlatformIO
- Arduino ESP32 Core
- PubSubClient
- Compilation Database (CLion compatible)

---

## 12. Future Enhancements

- TLS (MQTTS)
- HTTP authentication
- Persistent telemetry buffering
- NVS storage
- OTA updates
- Metrics endpoint
- Watchdog supervision

---

## 13. Author

Josemar Carvalho  
IoT / Embedded Systems Study Project
