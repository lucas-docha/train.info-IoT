#include <WiFi.h>
#include <PubSubClient.h>
#include a"env.h"

WiFiClient espclient;  //cria objeto p/ wifi
PubSubClient mqtt(espclient);

const String SSID = "FIESC_IOT_EDU"; 
const String PASS = "8120gv08";

const String brokerURL = "test.mosquitto.org";
const int Port = 1883;
const String topico = "TopicoCaio"; // tópico que será inscrito
const String topicoPub = "AulaDSM7"; // tópico para publicar as mensagens

const String brokerUser = "";
const String brokerPass = "";

void connectToWiFi();
void connectToBrker();


#define LED_PIN 2  // LED embutido no pino 2

void callback(char* topic, byte* payload, unsigned long lenght) {
  String msgRecebida = "";
  for(int i = 0; i < lenght; i++){
    msgRecebida += (char) payload[i];
  }
  Serial.print("Mensagem recebida no tópico ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(msgRecebida);

  // Controle do LED via mensagem MQTT, opcional:
  if (msgRecebida == "1") {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("LED ligado por MQTT");
  } else if (msgRecebida == "0") {
    digitalWrite(LED_PIN, LOW);
    Serial.println("LED desligado por MQTT");
  }
}

void setup() {

Serial.begin(115200);
pinMode(LED_PIN, OUTPUT); // configura o pino do LED como saída
digitalWrite(LED_PIN, LOW); // garante que o LED comece desligado

WiFi.begin(SSID, PASS);
Serial.println("Conectando no Wifi");
while(WiFi.status() != WL_CONNECTED) {
  Serial.print(".");
  delay(200);
}  
Serial.println("\nConectado com sucesso!");

mqtt.setServer(brokerURL.c_str(), Port); //configura o servidor do broker/porta
mqtt.setCallback(callback); // define a função de callback para receber mensagens
Serial.println("Conectando no Broker");

String boardID = "S1-"; // cria um nome que começa com "S1-"
boardID += String(random(0xffff),HEX); // junta o "S1-" com um número aleatório Hexadecimal

while(!mqtt.connect(boardID.c_str())){  // enquanto não estiver conectado mostra "."
  Serial.print(".");
  delay(200);
}

mqtt.subscribe(topico.c_str()); // inscreve no tópico
Serial.println("\nConectado com sucesso ao broker");
}

void loop() {
  if (!mqtt.connected()) {
    Serial.println("Reconectando ao broker...");
    mqtt.connect("S1-Recon");
    mqtt.subscribe(topico.c_str());
  }

  String msg = "";

  if(Serial.available() > 0) {
    msg = Serial.readStringUntil('\n');
    Serial.print("Mensagem digitada: ");
    Serial.println(msg);

    // Controle do LED pela Serial:
    if (msg == "1") {
      digitalWrite(LED_PIN, HIGH);
      mqtt.publish(topicoPub.c_str(), "LED ligado");
      Serial.println("LED ligado!");
    } 
    else if (msg == "0") {
      digitalWrite(LED_PIN, LOW);
      mqtt.publish(topicoPub.c_str(), "LED desligado");
      Serial.println("LED desligado!");
    } 
    else {
      // envia msg normal via MQTT
      String msgPub = "Caio: " + msg;
      mqtt.publish(topicoPub.c_str(), msgPub.c_str());
      Serial.println("Mensagem enviada ao MQTT!");
    }
  }

  mqtt.loop(); //mantem conexão
}