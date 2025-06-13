#include <WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";

const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;
const char* topic_nivel = "reservatorio/nivel";
const char* topic_bomba = "reservatorio/bomba";

// Pinos
const int PIN_TRIGGER = 5;
const int PIN_ECHO = 18;
const int PIN_RELE = 23;
const int PIN_LED = 2;
const int PIN_BOTAO = 13;  // Botão para forçar envio MQTT

WiFiClient espClient;
PubSubClient client(espClient);
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Endereço I2C comum do LCD

bool bomba_ligada = false;
float nivel_agua = 0.0;
const float altura_reservatorio = 40.0;

unsigned long lastMsgTime = 0;
const long interval = 2000;
unsigned long lastReconnectAttempt = 0;

void setup_wifi() {
  Serial.print("Conectando-se ao WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Conectado!");
}

float medirDistancia() {
  digitalWrite(PIN_TRIGGER, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIGGER, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIGGER, LOW);

  long duracao = pulseIn(PIN_ECHO, HIGH, 30000);
  if (duracao == 0) return -1;

  float distancia = (duracao * 0.0343) / 2.0;
  return distancia;
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  msg.toLowerCase();

  Serial.print("Mensagem recebida no tópico ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(msg);

  if (String(topic) == topic_bomba) {
    if (msg == "liga") {
      bomba_ligada = true;
      Serial.println("Bomba ligada!");
    } else if (msg == "desliga") {
      bomba_ligada = false;
      Serial.println("Bomba desligada!");
    }
  }
}

bool mqtt_reconnect() {
  if (client.connect("ESP32Reservatorio")) {
    Serial.println("Conectado ao MQTT!");
    client.subscribe(topic_bomba);
  } else {
    Serial.print("Falha na conexão MQTT, rc=");
    Serial.print(client.state());
    Serial.println(" tentando novamente em 5 segundos");
  }
  return client.connected();
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_TRIGGER, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_RELE, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BOTAO, INPUT_PULLUP);  // Botão com resistor de pull-up interno

  digitalWrite(PIN_RELE, LOW);
  digitalWrite(PIN_LED, LOW);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Iniciando...");

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      mqtt_reconnect();
    }
  } else {
    client.loop();

    bool botaoPressionado = digitalRead(PIN_BOTAO) == LOW;
    unsigned long now = millis();

    if (now - lastMsgTime > interval || botaoPressionado) {
      lastMsgTime = now;

      if (botaoPressionado) {
        Serial.println("Botão pressionado! Forçando leitura e envio...");
        delay(300); // debounce simples
      }

      digitalWrite(PIN_RELE, bomba_ligada ? HIGH : LOW);
      digitalWrite(PIN_LED, bomba_ligada ? HIGH : LOW);

      float distancia = medirDistancia();

      if (distancia < 0 || distancia > altura_reservatorio) {
        Serial.println("Distância fora de alcance ou erro na leitura!");
        nivel_agua = 0.0;
      } else {
        nivel_agua = altura_reservatorio - distancia;
      }

      Serial.print("Distância medida: ");
      Serial.print(distancia);
      Serial.print(" cm | Nível da água: ");
      Serial.print(nivel_agua);
      Serial.println(" cm");

      char nivelStr[8];
      dtostrf(nivel_agua, 4, 2, nivelStr);

      client.publish(topic_nivel, nivelStr, true);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Nivel: ");
      lcd.print(nivelStr);
      lcd.print(" cm");
      lcd.setCursor(0, 1);
      lcd.print("Bomba: ");
      lcd.print(bomba_ligada ? "Ligada" : "Desl.");
    }
  }
}
