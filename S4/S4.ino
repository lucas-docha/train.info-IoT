#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "env.h"

WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

const char* mqtt_server = BROKER_URL;
const int mqtt_port = BROKER_PORT;
const char* mqtt_user = BROKER_USER;
const char* mqtt_pass = BROKER_PASS;

const char* topic_comando_trem = TOPICO_CONTROLE_S4_TREM;
const char* topic_velocidade = TOPICO_S4_TREM_VEL;

#define IN1 25
#define IN2 26
#define PWM 27

#define PWM_CHANNEL 0
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8

String currentDirection = "PARAR";
int currentSpeed = 0;

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
    
    String clientId = "ESP32_S4_";
    clientId += String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("\nConectado com sucesso ao broker MQTT!");
      
      mqttClient.subscribe(topic_comando_trem);
      Serial.print("Inscrito no tópico: ");
      Serial.println(topic_comando_trem);
      
      mqttClient.publish(topic_velocidade, "PARAR");
      
    } else {
      Serial.print("falhou, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

void setMotor(String direction, int speed) {
  currentDirection = direction;
  currentSpeed = speed;
  
  if (direction == "FRENTE") {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    ledcWrite(PWM_CHANNEL, speed);
    Serial.print("Motor: FRENTE (Vel: ");
    Serial.print(speed);
    Serial.println(")");
  } else if (direction == "TRAS") {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    ledcWrite(PWM_CHANNEL, speed);
    Serial.print("Motor: TRAS (Vel: ");
    Serial.print(speed);
    Serial.println(")");
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    ledcWrite(PWM_CHANNEL, 0);
    Serial.println("Motor: PARAR");
  }
  
  String status = currentDirection + " (Vel: " + String(currentSpeed) + ")";
  mqttClient.publish(topic_velocidade, status.c_str());
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

  if (strcmp(topic, topic_comando_trem) == 0) {
    message.toUpperCase();
    
    int spaceIndex = message.indexOf(' ');
    String command = (spaceIndex != -1) ? message.substring(0, spaceIndex) : message;
    int speed = (spaceIndex != -1) ? message.substring(spaceIndex + 1).toInt() : 0;
    
    if (speed > 255) speed = 255;
    if (speed < 0) speed = 0;

    if (command == "FRENTE") {
      setMotor("FRENTE", speed);
    } else if (command == "TRAS") {
      setMotor("TRAS", speed);
    } else if (command == "PARAR" || command == "FREAR") {
      setMotor("PARAR", 0);
    } else {
      Serial.println("Comando de trem inválido. Use FRENTE [VEL], TRAS [VEL] ou PARAR.");
    }
  }
}

void setup() {
  Serial.begin(115200);
  randomSeed(micros()); 

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);  
  ledcAttachPin(PWM, PWM_CHANNEL);

  setMotor("PARAR", 0);

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
  
  delay(10);
}
