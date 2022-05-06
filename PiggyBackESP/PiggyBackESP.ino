#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
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

int state;

// Boolsk verdi som bestemmer om søppel er fult 
bool isFull = false;


// variabler for millis
float millis1;
float millis2;

// variabler for avstand
float total_distance;

// målinger for de siste 60 sek, de lagres som globale verdier som settes til null hvert 60. sek
// deklarerer midlertidige variabler
float sec_over_70;
float sec_over_70_current = 0;
float max_speed;
float max_speed_current;
float average_speed;
float average_speed_num;
int n = 0;

// Variabel for input fra Zumo32U4
int input;


////////////////////INNLESING AV OPPDATERINGER FRA ZUMO32U4 ///////////////////

typedef struct {
  float nowSpeed;
} update_array;

update_array updateArray[8];

////////////////////// TRASHCAN ///////////////////////////
float trash = 0;
bool trashFull = false;


////////////////////// Egendefinert funksjoner //////////////////////////
// Henter strømprisen
int get_current_price() {
  return 0;
}

// Henter saldo 
int get_account_balance() {
  return 0;
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


  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off". 
  // Changes the output state according to the message
  if (String(topic) == "esp2/charge") {
    Serial.println(messageTemp);
    batteryLevel = messageTemp.toInt();
    }

  if (String(topic) == "esp4/trash") {
    //trash = messageTemp.toInt();  // Konverterer fra String til int
    Serial.print("Trash: ");
    Serial.println(messageTemp);
    trash = messageTemp.toFloat();
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
      client.subscribe("esp4/trash");
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

// Funksjon som finner nytt batteri-nivå
float find_battery_level(){
  // Henter hastighet
  float speed_now = input;
  
  // Funksjon som finner ny batteriverdi som funksjon av hastighet
  float previous_battery_level = batteryLevel;
  batteryLevel = previous_battery_level-(speed_now)*0.90;
  
  return new_battery_level;
}

// Funksjon som oppdaterer batteri-helse 
float get_battery_health(){
  float previous_battery_health = battery_health;

  // Formel for batteri-helse
  battery_health = previous_battery_health - (sec_over_70+max_speed+average_speed)/(100)-(charging_cycles+times_under_5)/(100);
}

// Funksjon som finner tilstand til Zumo-bilen
int find_state(){
  // Om batterinivå er under 10%
  if(batteryLevel > 5 && batteryLevel <= 10){
    state = 2;
  } else if(batteryLevel <= 5){ // Om batterinivå er under 5%
    state = 3;
  }else if (batteryLevel == 0){ // Om batteriet er flatt
    state = 4;
  }else if (isFull == true){ // Om søppel er fult 
    state = 1;
  }else if (batteryLevel <= 30 && batteryLevel >= 20){ // Under 30% så skal Zumo lade 
    state = 0;
  }else if (batteryLevel == 0 || battery_health == 0){ // Bilen er tom for strøm eller batteri er ødelagt
    state = 5;
  }else{ // Om alt er ok skal bilen bare kjøre
    state = 6;
  }
}


////////////////////////// Void setup og loop /////////////////////////

void setup() {
 // Serial.begin(115200);
  Serial.begin(115200);
  
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // Trenger å printe noe slik at den serielle kommunikasjonen starter
  Serial.print(6);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  delay(100);

  // Oppdaterer batteri-nivå og -helse
  find_battery_level();
  get_battery_health();

  // Sjekker søppelkasse
  if (trash == 100) {
    // Setter isFull til true
    isFull = true;
  }

  char batteryString[8];
  dtostrf(batteryLevel, 1, 2, batteryString);
  client.publish("esp3/charge", batteryString);
  

  // Finner tilstand til Zumo
  find_state();

  // Venter 
  if (Serial.available() > 0) {
    input = Serial.read();

    Serial.print(find_state());
  }
  delay(300);
}
