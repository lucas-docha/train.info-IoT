#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Ultrasonic.h>
#include "env.h"

WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

const char* mqtt_server = BROKER_URL;
const int mqtt_port = BROKER_PORT;
const char* mqtt_user = BROKER_USER;
const char* mqtt_pass = BROKER_PASS;

const char* topic_presenca1 = TOPICO_S2_PRESENCA1;
const char* topic_presenca2 = TOPICO_S2_PRESENCA2;

#define ULTRASONIC_ECHO1 1
#define ULTRASONIC_TRIG1 2
#define ULTRASONIC_ECHO2 3
#define ULTRASONIC_TRIG2 4

Ultrasonic sensorEntrada(ULTRASONIC_TRIG1, ULTRASONIC_ECHO1);
Ultrasonic sensorSaida(ULTRASONIC_TRIG2, ULTRASONIC_ECHO2);

int limiteDistancia = 100 ;

long lastPublish = 0;
const long publishInterval = 500;

void setup_wifi() {
  delay(10);
  Serial.print("Conectando ao WiFi ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi conectado com sucesso!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Tentando conexão MQTT...");
    
    String clientId = "ESP32_S2_";
    clientId += String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("\nConectado com sucesso ao broker MQTT!");
      
    } else {
      Serial.print("falhou, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
}

void publishData(const char* topic, const char* payload) {
  if (mqttClient.publish(topic, payload)) {
    Serial.print("Publicado [");
    Serial.print(topic);
    Serial.print("]: ");
    Serial.println(payload);
  } else {
    Serial.println("ERRO: Falha ao publicar dado.");
  }
}

void setup() {
  Serial.begin(115200);
  randomSeed(micros()); 

  setup_wifi();

  espClient.setInsecure();
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(callback);
}

void loop() {
  if (!mqttClient.connected()) {
    reconnect();
  }
  
  mqttClient.loop();

  int distEntrada = sensorEntrada.read();
  int distSaida = sensorSaida.read();

  if (millis() - lastPublish > publishInterval) {
    lastPublish = millis();
    
    char payload1[10];
    itoa(distEntrada, payload1, 10);
    publishData(topic_presenca1, payload1);

    char payload2[10];
    itoa(distSaida, payload2, 10);
    publishData(topic_presenca2, payload2);
  }

  delay(10);
}
