#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>
#include "env.h"

WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

const char* mqtt_server = BROKER_URL;
const int mqtt_port = BROKER_PORT;
const char* mqtt_user = BROKER_USER;
const char* mqtt_pass = BROKER_PASS;

const char* topic_subscribe_led = TOPICO_CONTROLE_S1_LED_ILUMINACAO;
const char* topic_subscribe_rgb = TOPICO_CONTROLE_S1_LED_RGB;

#define DHTPIN 4
#define LDR_PIN 34
#define ULTRASONIC_ECHO 23
#define ULTRASONIC_TRIG 22
#define LED_PIN 19
#define RGB_R_PIN 14
#define RGB_G_PIN 26
#define RGB_B_PIN 25

#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

long lastMsg = 0;
const int publishInterval = 5000;

float readTemperature() {
  float t = dht.readTemperature();
  if (isnan(t)) return -999.0;
  return t; 
}

float readHumidity() {
  float h = dht.readHumidity();
  if (isnan(h)) return -999.0;
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

void setRgbColor(int r, int g, int b) {
  ledcWrite(0, r);
  ledcWrite(1, g);
  ledcWrite(2, b);
}

void publishLedState() {
  int ledState = digitalRead(LED_PIN);
  const char* state = (ledState == HIGH) ? "1" : "0";
  mqttClient.publish(topic_subscribe_led, state);
}

void publishSensorData(float value, const char* topic) {
  if (value == -999.0) return;
  char msg[10];
  dtostrf(value, 4, 2, msg);
  mqttClient.publish(topic, msg);
}

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

void reconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Tentando conexão MQTT...");
    
    String clientId = "ESP32_S1_";
    clientId += String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("\nConectado com sucesso ao broker MQTT!");
      
      mqttClient.subscribe(topic_subscribe_led);
      Serial.print("Inscrito no tópico: ");
      Serial.println(topic_subscribe_led);
      
      mqttClient.subscribe(topic_subscribe_rgb);
      Serial.print("Inscrito no tópico: ");
      Serial.println(topic_subscribe_rgb);
      
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

  if (strcmp(topic, topic_subscribe_led) == 0) {
    if (message == "1") {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("LED Iluminação LIGADO.");
    } else if (message == "0") {
      digitalWrite(LED_PIN, LOW);
      Serial.println("LED Iluminação DESLIGADO.");
    }
  }

  else if (strcmp(topic, topic_subscribe_rgb) == 0) {
    int r = message.substring(0, message.indexOf(',')).toInt();
    int g = message.substring(message.indexOf(',') + 1, message.lastIndexOf(',')).toInt();
    int b = message.substring(message.lastIndexOf(',') + 1).toInt();
    
    setRgbColor(r, g, b);
    Serial.print("LED RGB ajustado para R:");
    Serial.print(r);
    Serial.print(" G:");
    Serial.print(g);
    Serial.print(" B:");
    Serial.println(b);
  }
}

void setup() {
  Serial.begin(115200);
  randomSeed(micros());

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(ULTRASONIC_TRIG, OUTPUT);
  pinMode(ULTRASONIC_ECHO, INPUT);

  ledcSetup(0, 5000, 8);
  ledcAttachPin(RGB_R_PIN, 0);
  ledcSetup(1, 5000, 8);
  ledcAttachPin(RGB_G_PIN, 1);
  ledcSetup(2, 5000, 8);
  ledcAttachPin(RGB_B_PIN, 2);
  setRgbColor(0, 0, 0);

  dht.begin();

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
