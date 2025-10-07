#include <WiFi.h>
#include <PubSubClient.h>

WiFiClient client;  //cria objeto p/ wifi
PubSubClient mqtt(client);

const String SSID = "FIESC_IOT_EDU"; 
const String PASS = "8120gv08";

const String brokerURL = "test.mosquitto.org";
const int brokerPort = 1883;

const String brokerUser = "";
const String brokerPass = "";

void setup() {

Serial.begin(115200);
WiFi.begin(SSID, PASS);
Serial.println("Conectando no Wifi");
while(WiFi.status() != WL_CONNECTED) {
  Serial.print(".");
  delay(200);
}  
Serial.println("\nConectado com sucesso!");

mqtt.setServer(brokerURL.c_str(), brokerPort); //configura o servidor do broker/porta
Serial.println("Conectando no Broker");

String boardID = "S1-"; // cria um nome que começa com "S1-"
boardID += String(random(0xffff),HEX); // junta o "S1-" com um número aleatório Hexadecimal

while(!mqtt.connect(boardID.c_str())){  // enquanto não estiver conectado mostra "."
  Serial.print(".");
  delay(200);
}
Serial.println("\nConectado com sucesso ao broker");

}

void loop() {
  // put your main code here, to run repeatedly:

}
