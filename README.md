# Projeto ESP32 - Monitoramento de Nível de Água com Controle via MQTT

## Descrição

Este projeto utiliza um **ESP32** para monitorar o nível de água de um reservatório utilizando um sensor ultrassônico e controlar uma bomba de água remotamente via protocolo **MQTT**. As leituras são exibidas em um **display LCD 16x2** via comunicação **I2C** e os comandos da bomba podem ser acionados por mensagem MQTT.

## Componentes Utilizados

- ESP32
- Sensor Ultrassônico HC-SR04
- Relé 5V (controle da bomba)
- LED (indicador do estado da bomba)
- Botão físico (para forçar envio de leitura)
- Display LCD 16x2 com interface I2C
- Conexão Wi-Fi (SSID: `Wokwi-GUEST`)
- Broker MQTT público: `broker.emqx.io`

## Funcionamento

### 1. Conexão Wi-Fi

O ESP32 conecta-se à rede Wi-Fi especificada para possibilitar a comunicação com o broker MQTT.

### 2. Medição do Nível de Água

O sensor ultrassônico mede a distância entre a tampa do reservatório e o nível da água. Com base na **altura total do reservatório (40 cm)**, o nível real de água é calculado subtraindo-se essa distância da altura total.

```cpp
nivel_agua = altura_reservatorio - distancia;
