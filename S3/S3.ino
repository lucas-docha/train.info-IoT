#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Ultrasonic.h>
#include <Servo.h>
#include "env.h"

//Conexão com o mqtt segura
WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);
 
//Pinos para conectar com os sensores e led
#define ultrasonic_trig 2
#define ultrasonic_echo 4

#define LED_PIN 5

#define SERVO1_PIN 14
#define SERVO2_PIN 27

// Objetos dos sensores
Ultrasonic sensorEntrada(ultrasonic_trig, ultrasonic_echo);
Servo servo1;
Servo servo2;

// Distância para detectar o trem
int limiteDeteccao = 15;

//Void call back
void callback(char* topic, byte* payload, unsigned int length) {
  // Caso queira receber comandos futuramente
}

//Reconexão
void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Conectando ao MQTT... ");

    if (mqttClient.connect("ESP32_TREM", MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("Conectado!");
      mqttClient.subscribe("trem/comandos");
    } else {
      Serial.print("Falha: ");
      Serial.println(mqttClient.state());
      delay(2000);
    }
  }
}


void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);

  // Servos
  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);
  servo1.write(0);
  servo2.write(0);

  // WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando ao WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" conectado!");

  // SSL 
  espClient.setInsecure();

  // MQTT
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(callback);
}


void loop() {
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();

  // Leitura da distância
  long distancia = sensorEntrada.distanceRead();

  //Repassa os dados
  char mensagemDist[10];
  sprintf(mensagemDist, "%ld", distancia);
  mqttClient.publish(MQTT_TOPIC_DISTANCIA, mensagemDist);

  Serial.print("Distância: ");
  Serial.println(distancia);

  if (distancia > 0 && distancia <= limiteDeteccao) {
    digitalWrite(LED_PIN, HIGH);

    // Mudar trilhos
    servo1.write(90);
    servo2.write(90);

    mqttClient.publish(MQTT_TOPIC_STATUS, "Trem detectado");
    mqttClient.publish(MQTT_TOPIC_TRILHO, "Desvio ativado");

    delay(1000);
  } 
  else {
    digitalWrite(LED_PIN, LOW);

    // Trilho padrão
    servo1.write(0);
    servo2.write(0);

    mqttClient.publish(MQTT_TOPIC_STATUS, "Sem trem");
    mqttClient.publish(MQTT_TOPIC_TRILHO, "Trilho normal");
  }

  delay(200);
}
