# WiFiManager (ESP32 / Arduino)

Biblioteca pequena para **gerenciar conexão Wi‑Fi em modo Station (STA)** no ESP32 usando o framework **Arduino**.

Ela fornece uma API simples e previsível para:

- configurar o ESP32 como STA com `hostname`, `sleep` e `txPower`
- **conectar com timeout** (sem bloquear indefinidamente)
- manter o estado da conexão e **tentar reconectar periodicamente** (sem flood)
- imprimir informações de rede (IP, gateway, MAC, RSSI)

> Observação: apesar do nome, esta biblioteca **não é um “WiFiManager” estilo portal cativo**. Ela não cria AP para configuração e não salva credenciais em NVS. Ela apenas encapsula o fluxo STA comum.

---

## Como a autenticação Wi‑Fi funciona aqui

A “autenticação” desta biblioteca é a autenticação do **Wi‑Fi do Access Point (AP)** (WPA/WPA2/WPA3), feita pela **pilha Wi‑Fi do Arduino‑ESP32**.

- Você informa `ssid` e `pass`.
- O ESP32 executa o *handshake* e a negociação de chaves (por exemplo, WPA2‑PSK), de acordo com as capacidades do AP.
- Se o AP aceitar, o ESP32 recebe um **IP via DHCP** e o evento de “got IP” é disparado.

A biblioteca:

- **não implementa criptografia própria**
- **não faz autenticação de aplicação** (isso é assunto de libs como SecureHttp)
- apenas controla o **estado da conexão** e a política de reconexão

---

## O que a biblioteca faz

### `begin()`

- `WiFi.mode(WIFI_STA)`
- `WiFi.setHostname(...)`
- `WiFi.setSleep(...)`
- `WiFi.setTxPower(...)`
- registra um callback de evento (`WiFi.onEvent(...)`) e encaminha para a instância

### `connect()`

- imprime logs de conexão
- limpa estado anterior com `WiFi.disconnect(true)`
- chama `WiFi.begin(ssid, pass)`
- espera até `WL_CONNECTED` ou atingir `connectTimeoutMs`

### `update()`

- roda leve, pensado para ser chamado no `loop()`
- a cada 5s verifica se caiu
- se estiver desconectado, chama `WiFi.reconnect()`

### `isConnected()`

- retorna verdadeiro apenas quando:
  - `_connected == true` **e**
  - `WiFi.status() == WL_CONNECTED`

### `printNetInfo()`

- imprime IP, gateway, MAC e RSSI

---

## Como usar

### Exemplo mínimo

```cpp
#include <Arduino.h>
#include <WiFiManager.h>

WiFiManager* wifi;

void setup() {
  Serial.begin(115200);

  WiFiManager::Config cfg;
  cfg.ssid = "MinhaRede";
  cfg.pass = "MinhaSenha";
  cfg.hostname = "device-esp32";
  cfg.connectTimeoutMs = 20000;
  cfg.sleep = true;
  cfg.txPower = WIFI_POWER_8_5dBm;

  wifi = new WiFiManager(cfg);
  wifi->begin();
  wifi->connect();
}

void loop() {
  wifi->update();

  if (wifi->isConnected()) {
    // faça chamadas HTTP/MQTT etc.
  }

  delay(10);
}
```

### Configuração (`WiFiManager::Config`)

- `ssid`: SSID do AP (obrigatório)
- `pass`: senha do AP (obrigatório)
- `hostname`: nome DHCP (default: `gateway-arduino`)
- `connectTimeoutMs`: timeout do `connect()` (default: 20000)
- `sleep`: habilita Wi‑Fi sleep (default: `true`)
- `txPower`: potência TX do rádio (default: `WIFI_POWER_8_5dBm`)

---

## Integração com PlatformIO

Se você está usando esta lib em `shared-libs`, normalmente ela é consumida via `lib_extra_dirs`.

Exemplo (`platformio.ini`):

```ini
lib_extra_dirs =
  ../shared-libs
```

Ou coloque a pasta da lib dentro de `lib/` do projeto.

---

## Limitações e notas

- **Uma instância por vez:** a implementação usa um ponteiro estático (`_self`) para encaminhar eventos. Se criar duas instâncias, a última sobrescreve.
- Eventos Wi‑Fi: este código usa `SYSTEM_EVENT_*`. Dependendo da versão do Arduino‑ESP32, os enums podem mudar (por exemplo, `ARDUINO_EVENT_WIFI_STA_*`). Se você atualizar o core e quebrar, a correção é adaptar os cases de evento.
- Reconexão: `update()` tenta `WiFi.reconnect()` a cada 5s. Se você quiser política mais agressiva ou backoff exponencial, isso pode ser expandido.

---

## Documentação (Doxygen)

### Requisitos

- `doxygen`
- (opcional) `graphviz` para gráficos

### Gerar HTML

Na raiz da lib:

```bash
doxygen Doxyfile
```

A saída padrão vai para:

- `docs/doxygen/html/index.html`

---

## Licença

Defina conforme o repositório do projeto.

