#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Ultrasonic.h>
#include <Servo.h>
#include "env.h"

WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

const char* mqtt_server = BROKER_URL;
const int mqtt_port = BROKER_PORT;
const char* mqtt_user = BROKER_USER;
const char* mqtt_pass = BROKER_PASS;

const char* topic_presenca = TOPICO_S3_PRESENCA;
const char* topic_servo1 = TOPICO_CONTROLE_S3_SERVO1;
const char* topic_servo2 = TOPICO_CONTROLE_S3_SERVO2;

#define ULTRASONIC_TRIG 2
#define ULTRASONIC_ECHO 4
#define SERVO1_PIN 14
#define SERVO2_PIN 27

Ultrasonic sensorEntrada(ULTRASONIC_TRIG, ULTRASONIC_ECHO);
Servo servo1;
Servo servo2;

int limiteDeteccao = 15;

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
    
    String clientId = "ESP32_S3_";
    clientId += String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("\nConectado com sucesso ao broker MQTT!");
      
      mqttClient.subscribe(topic_servo1);
      Serial.print("Inscrito no tópico: ");
      Serial.println(topic_servo1);
      
      mqttClient.subscribe(topic_servo2);
      Serial.print("Inscrito no tópico: ");
      Serial.println(topic_servo2);
      
    } else {
      Serial.print("falhou, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem recebida no tópico [");
  Serial.print(topic);
  Serial.print("]: ");
  
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  int angle = message.toInt();
  
  if (strcmp(topic, topic_servo1) == 0) {
    if (angle >= 0 && angle <= 180) {
      servo1.write(angle);
      Serial.print("Servo 1 ajustado para: ");
      Serial.println(angle);
    } else {
      Serial.println("Comando inválido para Servo 1. Use ângulo entre 0 e 180.");
    }
  }
  
  else if (strcmp(topic, topic_servo2) == 0) {
    if (angle >= 0 && angle <= 180) {
      servo2.write(angle);
      Serial.print("Servo 2 ajustado para: ");
      Serial.println(angle);
    } else {
      Serial.println("Comando inválido para Servo 2. Use ângulo entre 0 e 180.");
    }
  }
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

  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);
  servo1.write(0);
  servo2.write(0);

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

  long distancia = sensorEntrada.read();

  if (millis() - lastPublish > publishInterval) {
    lastPublish = millis();
    
    char payload[10];
    itoa(distancia, payload, 10);
    publishData(topic_presenca, payload);
  }

  delay(10);
}
