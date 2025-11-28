#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "env.h"

//Pinos
#define IN1 25
#define IN2 26
#define PWM 27

//Conexão MQTT
WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);


void tremFrente(int velocidade) {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  ledcWrite(0, velocidade);  
  mqttClient.publish(MQTT_TOPIC_STATUS, "Trem andando para frente");
}

void tremTras(int velocidade) {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  ledcWrite(0, velocidade);
  mqttClient.publish(MQTT_TOPIC_STATUS, "Trem andando para trás");
}

void tremFrear() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, HIGH);
  ledcWrite(0, 0);
  mqttClient.publish(MQTT_TOPIC_STATUS, "Trem FREIO acionado");
}

//CallBack
void callback(char* topic, byte* payload, unsigned int length) {
  String comando = "";

  for (int i = 0; i < length; i++) {
    comando += (char)payload[i];
  }

  comando.trim();

  if (comando == "frente") {
    tremFrente(200); 
  }
  else if (comando == "tras") {
    tremTras(200);
  }
  else if (comando == "frear") {
    tremFrear();
  }
}

//Reconexão MQTT
void reconnectMQTT() {
  while (!mqttClient.connected()) {
    if (mqttClient.connect("Trem_S4", MQTT_USER, MQTT_PASSWORD)) {
      mqttClient.subscribe(MQTT_TOPIC_CMD);
    } else {
      delay(2000);
    }
  }
}


void setup() {
  Serial.begin(115200);

  // Configuração dos pinos
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  ledcSetup(0, 5000, 8);  
  ledcAttachPin(PWM, 0);

  
  tremFrear();

  // WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  espClient.setInsecure();
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(callback);
}

void loop() {
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();
}
