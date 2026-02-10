# curso-iot-cpqd-fiap

## Visão Geral

O **curso-iot-cpqd-fiap** é um projeto educacional e técnico desenvolvido no contexto do curso de IoT (FIAP / CPQD),
com foco em **arquitetura IoT segura, modular e extensível**, utilizando **ESP32**, comunicação HTTP segura,
integração com serviços em nuvem e boas práticas de engenharia de software embarcado.

O projeto simula um cenário real de telemetria veicular/industrial, com separação clara entre:

- **Dispositivo de campo (vehicle-device)**
- **Gateway IoT (gateway-arduino)**
- **Bibliotecas compartilhadas (shared-libs)**

---

## Arquitetura Geral

```
+------------------+        HTTPS (SecureHttp)        +---------------------+
|                  |  ----------------------------> |                     |
|  vehicle-device  |                                 |   gateway-arduino   |
|  (ESP32)         |                                 |   (ESP32 / Wi-Fi)   |
|                  | <-----------------------------  |                     |
+------------------+                                  +----------+----------+
                                                                  |
                                                                  |
                                            +---------------------+---------------------+
                                            |                                           |
                                      Ubidots (MQTT/HTTP)                         ThingSpeak (HTTP)
```

---

## Componentes do Projeto

### 1. vehicle-device

Dispositivo embarcado responsável por:

- Leitura de sensores físicos e simulados:
  - Temperatura e umidade (DHT22)
  - Nível de combustível (ADC)
  - Aceleração e RPM simulados (potenciômetro)
- Aplicação de filtros (EMA)
- Construção de payload JSON
- Envio de telemetria **segura** ao gateway

🔐 Comunicação protegida via **SecureHttp**.

---

### 2. gateway-arduino

Gateway responsável por:

- Receber requisições HTTP do device
- Validar autenticação, integridade e replay
- Descriptografar payload
- Normalizar telemetria
- Encaminhar dados para:
  - **Ubidots** (imediato)
  - **ThingSpeak** (rate-limited)

Também atua como ponto central de controle e observabilidade.

---

### 3. shared-libs

Conjunto de bibliotecas reutilizáveis, projetadas para uso em múltiplos projetos ESP32:

- **SecureHttp**
  - Autenticação forte
  - AES-256-GCM + HMAC-SHA256
  - Proteção contra replay
- **WiFiManager**
  - Gerenciamento de conexão Wi-Fi
  - Reconexão automática
- **HttpServer**
  - Endpoints REST
  - Rate-limit
- **LedStatus**
  - Sinalização visual de estados
- **UbidotsClient / ThingSpeakClient**
  - Integração com nuvem

---

## Segurança (SecureHttp)

A segurança é um dos pilares centrais do projeto.

### Técnicas utilizadas

- **Criptografia:** AES-256-GCM
- **Autenticação:** HMAC-SHA256
- **Anti-replay:** Timestamp + Nonce
- **Integridade:** Tag GCM + assinatura
- **AAD:** Metadados autenticados (device, path, método, nonce)

### Benefícios

- Confidencialidade dos dados
- Integridade garantida
- Autenticidade do dispositivo
- Proteção contra ataques de replay
- Independência de TLS (ideal para IoT)

---

## Estrutura de Diretórios

```
curso-iot-cpqd-fiap/
├── gateway-arduino/
│   ├── src/
│   └── secrets.h
├── vehicle-device/
│   ├── src/
│   └── secrets.h
└── shared-libs/
    ├── SecureHttp/
    ├── WiFiManager/
    ├── LedStatus/
    ├── HttpServer/
    └── ...
```

---

## Fluxo de Execução (Resumo)

1. vehicle-device inicializa sensores e Wi-Fi
2. Coleta dados e gera JSON
3. Criptografa e assina payload
4. Envia POST `/telemetry`
5. gateway valida e descriptografa
6. Telemetria é registrada e publicada na nuvem

---

## Objetivos Educacionais

Este projeto demonstra, de forma prática:

- Arquitetura IoT realista
- Segurança aplicada em sistemas embarcados
- Modularização e reuso de código
- Integração edge → cloud
- Boas práticas de firmware ESP32

---

## Público-Alvo

- Estudantes de IoT e Sistemas Embarcados
- Desenvolvedores ESP32
- Engenheiros de Software / Firmware
- Projetos acadêmicos (FIAP / CPQD)

---

## Observações Importantes

⚠️ Arquivos `secrets.h` **não devem ser versionados**  
⚠️ Chaves e tokens são apenas exemplos  
⚠️ Projeto voltado para fins didáticos e POCs

---

## Próximos Passos (Evolução)

- MQTT seguro (TLS)
- Provisionamento dinâmico de chaves
- OTA seguro
- Dashboard próprio
- Persistência local no gateway
- Certificados por dispositivo

---

## Licença

Projeto educacional desenvolvido no contexto do curso IoT – FIAP / CPQD.
