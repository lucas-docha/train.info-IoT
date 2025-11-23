#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include "env.h"

// wificlient seguro
WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

// tópicos
const char* mqtt_topic_subscribe_led = TOPICO_CONTROLE_GERAL;
const char* mqtt_topic_subscribe_rgb = "SA_GRUPO_HMLC/Controle/RGB";

// pinos
#define DHTPIN 4
#define LDR_PIN 34
#define ULTRASONIC_ECHO 23
#define ULTRASONIC_TRIG 22
#define LED_PIN 19
#define RGB_R_PIN 14
#define RGB_G_PIN 26
#define RGB_B_PIN 25

// sensor DHT
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// controle publicação
long lastMsg = 0;
const int publishInterval = 5000;

// leitura sensores
float readTemperature() {
  float t = dht.readTemperature();
  return t; 
}

float readHumidity() {
  float h = dht.readHumidity();
  return h;
}

int readLuminosity() {
  return analogRead(LDR_PIN);
}

float readDistance() {
  digitalWrite(ULTRASONIC_TRIG, LOW);
  delayMicroseconds(2);

  digitalWrite(ULTRASONIC_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_TRIG, LOW);

  long duration = pulseIn(ULTRASONIC_ECHO, HIGH);
  return duration * 0.034 / 2;
}

// led rgb
void setRgbColor(int r, int g, int b) {
  digitalWrite(RGB_R_PIN, r > 0 ? HIGH : LOW);
  digitalWrite(RGB_G_PIN, g > 0 ? HIGH : LOW);
  digitalWrite(RGB_B_PIN, b > 0 ? HIGH : LOW);
}

// publicação
void publishLedState() {
  int ledState = digitalRead(LED_PIN);
  const char* state = (ledState == HIGH) ? "ON" : "OFF";
  mqttClient.publish(TOPICO_STATUS_LED, state);
}

void publishSensorData(float value, const char* topic) {
  char msg[10];
  dtostrf(value, 4, 2, msg);
  mqttClient.publish(topic, msg);
}

// conexão wifi
void setup_wifi() {
  Serial.print("Conectando ao WiFi ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// callback MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  // controle led normal
  if (strcmp(topic, mqtt_topic_subscribe_led) == 0) {
    if (message == "1") digitalWrite(LED_PIN, HIGH);
    if (message == "0") digitalWrite(LED_PIN, LOW);
    publishLedState();
  }

  // controle led rgb
  else if (strcmp(topic, mqtt_topic_subscribe_rgb) == 0) {
    int r = message.substring(0, message.indexOf(',')).toInt();
    int g = message.substring(message.indexOf(',') + 1, message.lastIndexOf(',')).toInt();
    int b = message.substring(message.lastIndexOf(',') + 1).toInt();
    setRgbColor(r, g, b);
  }
}

// setup
void setup() {
  Serial.begin(115200);
  randomSeed(micros());

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(RGB_R_PIN, OUTPUT);
  pinMode(RGB_G_PIN, OUTPUT);
  pinMode(RGB_B_PIN, OUTPUT);
  setRgbColor(0, 0, 0);

  pinMode(ULTRASONIC_TRIG, OUTPUT);
  pinMode(ULTRASONIC_ECHO, INPUT);

  dht.begin();
  setup_wifi();

  // tls
  espClient.setInsecure();

  // MQTT
  mqttClient.setServer(BROKER_URL, BROKER_PORT);
  mqttClient.setCallback(callback);

  Serial.println("Conectando ao broker MQTT com TLS...");

  if (mqttClient.connect("ESP32Client", BROKER_USER, BROKER_PASS)) {
    Serial.println("Conectado ao MQTT!");
    mqttClient.subscribe(mqtt_topic_subscribe_led);
    mqttClient.subscribe(mqtt_topic_subscribe_rgb);
  } else {
    Serial.println("ERRO: não conectou ao MQTT.");
  }
}

// loop
void loop() {
  mqttClient.loop();

  unsigned long now = millis();
  if (now - lastMsg > publishInterval) {
    lastMsg = now;

    float temp = readTemperature();
    float umid = readHumidity();
    int iluminacao = readLuminosity();
    float distancia = readDistance();

    publishSensorData(temp, TOPICO_S1_TEMP);
    publishSensorData(umid, TOPICO_S1_HUMID);
    publishSensorData(iluminacao, TOPICO_S1_ILUMINACAO);
    publishSensorData(distancia, TOPICO_S1_PRESENCA);
  }

  delay(10);
}
