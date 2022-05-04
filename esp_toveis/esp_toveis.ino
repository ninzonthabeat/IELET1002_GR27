#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <EEPROM.h>
#include <math.h>

// Replace the next variables with your SSID/Password combination
const char* ssid = "Nina";
const char* password = "orbit123";

// Add your MQTT Broker IP address, example:
//const char* mqtt_server = "192.168.1.144";
//const char* mqtt_server = "pecanpie.is-very-sweet.org";
const char* mqtt_server = "172.20.10.3";

WiFiClient espClient;
PubSubClient client(espClient);

long lastMsg = 0;
char msg[50];
int value = 0;

//////////////////// BATTERI ///////////////////////////
int batteryLevel = 100;
int battery_health = 0;
int charging_cycles = 0;
int times_under_5;

int current_price = 0;  // strømpris
int balance = 0;  // saldo

float new_battery_level;
bool batteryFull = true;   //om batteri er fullt?

// variabler for millis
float millis1;
float millis2;

// variabler for avstand
float total_distance;

// variabler for økonomi
int charging_cost;
int cost_per = 10; 

// målinger for de siste 60 sek, de lagres som globale verdier som settes til null hvert 60. sek
// deklarerer midlertidige variabler
float sec_over_70;
float sec_over_70_current = 0;
float max_speed;
float max_speed_current;
float average_speed;
float average_speed_num;
int n = 0;

// EEPROM adresser
int addr_5 = 0;
int addr_health = 1;

/*
 *  BATTERIKODE
 * 
 */

// Henter strømprisen
int get_current_price() {
  return 0;
}

// Henter saldo 
int get_account_balance() {
  return 0;
}

////////////////////INNLESING AV OPPDATERINGER FRA ZUMO32U4 ///////////////////

typedef struct {
  float nowSpeed;
} update_array;

update_array updateArray[8];

////////////////////// TRASHCAN ///////////////////////////
float trash = 0;
bool trashFull = false;


void setup() {
 // Serial.begin(115200);
  Serial.begin(115200);
  
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // Hent verdier fra EEPROM
  battery_health = EEPROM.read(addr_health);
  times_under_5 = EEPROM.read(addr_5);
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

// Callback is executed when some device publishes a message to a topic that your ESP32 is subscribed to.
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

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off". 
  // Changes the output state according to the message
  if (String(topic) == "esp2/charge") {
    Serial.println(messageTemp);
    }

  if (String(topic) == "esp4/trashOut") {
    //trash = messageTemp.toInt();  // Konverterer fra String til int
    Serial.print("Trash: ");
    Serial.println(messageTemp);
    
    }
  }


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP3Client")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp4/trashOut");
      client.subscribe("esp2/charge");
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
  
  delay(1000);

    //Serial.println("Testing node-RED");

  float battery = 12;
  

    
  char batteryString[8];
  dtostrf(battery, 1, 2, batteryString);
  //Serial.print("Battery level: ");
  //Serial.println(batteryString);
  
  //client.publish("esp3/battery", batteryString);
  //delay(5000);

  
  // Etterspør fart fra Zumo32U4
  Serial.print(1);

  

  // Venter 
  while(Serial.available() == 0) {
  }

  int speed = Serial.read();

  char speedString[8];
  dtostrf(speed, 1, 2, speedString);
  //Serial.print("Speed: ");
  //Serial.println(speedString);
  client.publish("esp3/speed", speedString);
  
  
  delay(3000);

  // Sjekker søppelkasse
  
}
