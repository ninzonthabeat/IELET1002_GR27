#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
 
// Replace the next variables with your SSID/Password combination
const char* ssid = "Nina";
const char* password = "orbit123";
 
// Add your MQTT Broker IP address, example:
//const char* mqtt_server = "192.168.1.144";
const char* mqtt_server = "172.20.10.3";
 
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
 
 float trash = 0;
bool isFull = false;
 
void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}
 
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
 
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// Kjører når data kommer inn
void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
   
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
 


// fyller opp trashcan
void trashcan() {

  if (trash == 100) {
    isFull = true;  // Setter full
    
  } else {
    trash += 5; // Inkrementerer hvis søppelkasse ikke er full
  }
}


 
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP4Client")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp4/trash");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // fyller søpla hvis den ikke er full
  if (isFull == false) {
    trashcan();
    delay(5000); 
  } else {
    // TESTKODE
    trash = 0;
    isFull = false;
  }
 
  long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;

    // Konverterer verdier til en char array
    char trashString[8];
    dtostrf(trash, 1, 2, trashString);
    Serial.print("Trash: ");
    Serial.println(trashString);
    Serial.println("");
    client.publish("esp4/trash", trashString);
    Serial.print("isFull: ");
    Serial.println(isFull);
  }
}
