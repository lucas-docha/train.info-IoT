#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "env.h"

// --- WiFi & MQTT Configuration ---
const char* brokerURL = BROKER_URL;
const int brokerPort = BROKER_PORT;
const char* mqttTopic = TOPICO_1;  // Tópico usado para publish/subscribe

WiFiClientSecure client;
PubSubClient mqtt(client);

const int ledPin = 19;  // Pino do LED embutido

// --- Função para receber mensagens MQTT ---
void callback(char* topic, byte* payload, unsigned int length) {
  String msgRecebida = "";
  for (unsigned int i = 0; i < length; i++) {
    msgRecebida += (char)payload[i];
  }

  Serial.print("Mensagem recebida via MQTT: ");
  Serial.println(msgRecebida);

  // Verifica se é um comando para o LED
  if (msgRecebida == "1") {
    digitalWrite(ledPin, HIGH);
    Serial.println("LED ligado via MQTT");
    mqtt.publish(mqttTopic, "LED ligado via MQTT");
  } else if (msgRecebida == "0") {
    digitalWrite(ledPin, LOW);
    Serial.println("LED desligado via MQTT");
    mqtt.publish(mqttTopic, "LED desligado via MQTT");
  }
}

void setup() {
  Serial.begin(115200);
  client.setInsecure();  // Conexão sem certificado (para testes)
  
  pinMode(ledPin, OUTPUT);

  // Conexão Wi-Fi
  Serial.print("Conectando ao WiFi ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\n Wi-Fi conectado com sucesso!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());

  // Conexão MQTT
  mqtt.setServer(brokerURL, brokerPort);
  mqtt.setCallback(callback);

  Serial.println("Conectando ao broker MQTT...");
  String boardID = "ESP32-" + String(random(0xffff), HEX);

  while (!mqtt.connect(boardID.c_str(), BROKER_USER, BROKER_PASS)) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\n Conectado ao broker MQTT!");
  mqtt.subscribe(mqttTopic);
  Serial.println("Inscrito no tópico: " + String(mqttTopic));
}

void loop() {
  mqtt.loop();  // Mantém a conexão MQTT ativa

  // Lê comandos pela Serial
  if (Serial.available()) {
    String comando = Serial.readStringUntil('\n');
    comando.trim();

    if (comando == "1") {
      digitalWrite(ledPin, HIGH);
      Serial.println("LED ligado via Serial");
      mqtt.publish(mqttTopic, "LED ligado via Serial");
    } else if (comando == "0") {
      digitalWrite(ledPin, LOW);
      Serial.println("LED desligado via Serial");
      mqtt.publish(mqttTopic, "LED desligado via Serial");
    } else {
      Serial.println("Comando inválido. Use 1 (ligar) ou 0 (desligar).");
    }
  }
}
