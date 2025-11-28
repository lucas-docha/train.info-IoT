#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Ultrasonic.h>
#include "env.h"

// Conexão com o MQTT
WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

// Pinos dos sensores ultrassônicos
#define ultrasonic_echo 1
#define ultrasonic_trig 2
#define ultrasonic_echo2 3
#define ultrasonic_trig2 4

// Pinos do LED
#define led_pin 5

// Objetos dos sensores
Ultrasonic sensorEntrada(ultrasonic_trig, ultrasonic_echo);
Ultrasonic sensorSaida(ultrasonic_trig2, ultrasonic_echo2);

// Distância para detectar trem 
int limiteDistancia = 100 ; // cm

// Verifica se o trem está no local
bool tremNaArea = false;

// Função callback do MQTT
void callback(char* topic, byte* payload, unsigned int length) {
}

void setup() {
  Serial.begin(115200);

  // Configurar pino do LED
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);

  // Iniciar WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando ao WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");

//Configuração tls e mqtt
  espClient.setInsecure(); 
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(callback);

  // Conectar ao MQTT
  while (!mqttClient.connected()) {
    Serial.println("Conectando ao MQTT...");
    if (mqttClient.connect("esp32_trem", MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("Conectado ao MQTT!");
    } else {
      Serial.print("Falhou, rc=");
      Serial.print(mqttClient.state());
      delay(1000);
    }
  }
}

void loop() {
  mqttClient.loop();

  // Ler distância dos dois sensores
  int distEntrada = sensorEntrada.read();
  int distSaida = sensorSaida.read();

  Serial.print("Entrada: ");
  Serial.print(distEntrada);
  Serial.print(" cm | Saída: ");
  Serial.println(distSaida);

  // DETECTOU O TREM ENTRANDO NA ÁREA
  if (distEntrada < limiteDistancia && !tremNaArea) {
    tremNaArea = true;

    digitalWrite(led_pin, HIGH);   // acende LED
    mqttClient.publish("ferrovia/trem", "Trem entrando na área");

    Serial.println("Trem detectado na entrada!");
    delay(500); // evitar múltiplas leituras
  }

  // DETECTOU O TREM SAINDO DA ÁREA
  if (distSaida < limiteDistancia && tremNaArea) {
    tremNaArea = false;

    digitalWrite(led_pin, LOW);   // apaga LED
    mqttClient.publish("ferrovia/trem", "Trem saiu da área");

    Serial.println("Trem detectado na saída!");
    delay(500);
  }

  delay(200);
}
