// LADESTASJONS KODE esp2/charge prøver med activate_charge som bool
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display bredde, i piksler
#define SCREEN_HEIGHT 64 // OLED display høyde, i piksler

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Skriver inn ssid og passord på nettet vi deler for å koble oss till
const char* ssid = "x";
const char* password = "x";

// Navnet på R-Pien vår som er host for MQTT-serveren
const char* mqtt_server = "pecanpie.is-very-sweet.org";


WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

// Deklarerer ulike variabler nedenfor

int batterycharge = 0;
int choose_charge;

int percentage_value = 0;

int price_count = 0;
int charging_price = 0;
int your_total_payment;
int bank_balance = 800;

int repair_charging_station = 0;

unsigned long time_now;

unsigned long time_out_while_time;
unsigned long previous_time_out_while_time;

int time_input = 0;
unsigned long time_input_in_while;
unsigned long previous_time_input_in_while;

// Verdier som jeg bruker i while løkker istedenfor delay for å ha muligheten til å registrere tasteinntrykk
int five_sec = 5000;
int twenty_sec = 20000;
int sixty_seconds = 60000;

int time_period = 100000; // Tids periode for reperasjon av ladestasjonen

int current_people_charging = 0;
int current_ampere  = 100;

bool stop_charge = false; // Setter disse til å være en bool, lettere enn tall som 1 eller 0.
bool activate_charge = false; 


void setup_wifi() {
  delay(10);
  // Tilkobler oss et WiFi nettverk
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

  if (String(topic) == "esp3/balance") 
  {
    batterycharge = messageTemp.toInt();
    Serial.print("batterycharge = ");
    Serial.println(activate_charge);
  }

  if (String(topic) == "esp2/activate") 
  {
    activate_charge = messageTemp.toInt();
    Serial.print("activate charge = ");
    Serial.println(activate_charge);
  }

  if (String(topic) == "esp3/balance") 
  {
    bank_balance = messageTemp.toInt();
    Serial.print("bank balance = ");
    Serial.println(bank_balance);
  }

  if (String(topic) == "esp2/charge") 
  {
    choose_charge = messageTemp.toInt();

    Serial.print("choose charge = ");
    Serial.println(choose_charge);

    switch (choose_charge)
    {
      case 0:
        full_charge();
        break;
      
      case 1:
        percentage_charge();
        break;
      
      case 2:
        stop_charge();
        break;
      
      case 3:
        timed_charge();
        break;
      
      case 4:
        not_enough_money_charge();
        break;
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("esp2")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp32/output");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void reset_variables() // Resetter variabel navn i slutten av void loop().
{
  activate_charge = false;
  choose_charge = 0;
  price_count = 0;
}


void time_out()
{ 
  for (int i = 0; i < 60; i++) // Ladestasjonen er ute av drift i seksti sekunder
  {
    activate_charge = 0;
  
    display.clearDisplay();
  
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 10);

    display.println("Out of order!");
    display.display();
    delay(1000);
  } 
}

void people_charging() // Tillegsmodul i ladestasjonen. Jo flere folk som lader jo mindre strøm får man ut og jo lengre tid tar det å lade.
{
  current_people_charging = random(0, 10);

  if (current_people_charging >= 8)
  {
    current_ampere = 50;
  }
  else if ((current_people_charging >= 5) && (current_people_charging < 8))
  {
    current_ampere = 70;
  }
  else if ((current_people_charging >= 3) && (current_people_charging < 5))
  {
    current_ampere = 85;
  }
  else
  {
    current_ampere = 100;
  }
}


void display_bank_balance() // Vises saldo til kunden på displayet
{
  for (int i = 0; i < 5; i++)
  {
    display.clearDisplay();

    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 10);

    display.println("Your current balance is: "); 
    display.println(" ");
    display.print(bank_balance); // Viser hvor kundens saldo
    display.print(" Kr");

    display.display();
    delay(1000);
  }
}

void display_showing_charge() // Viser når man lader
{
  batterycharge = batterycharge + 1;
  price_count = price_count + 1;

  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);

  display.println("Currently charging..."); 
  display.println(" ");
  display.print(batterycharge); // Viser hvor mange prosent oppladet bilen er
  display.print(" % ");
  display.println(" ");
  display.println("Other people ");
  display.println("charging: ");
  display.println(current_people_charging); // Viser hvor mange totalt som lader
  display.display();

  delay(time_period / current_ampere); 
}

void display_fully_charged() // Vises når bilen er fulladet
{ 
  for (int i = 0; i < 5; i++)
  {
    display.clearDisplay();
  
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 10);
  
    display.println("Fully charged!");
    display.println(" ");
    display.print(batterycharge);
    display.print(" % ");
    display.display();

    delay(1000);
  }
}


void your_total_is() // Viser summen av hva ladingen kostet
{
  for (int i = 0; i < 5; i++)
  {
    display.clearDisplay();
    
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 10);

    display.println("Your total is: ");
    your_total_payment = charging_price * price_count; // charging_price er fast under ladingen og price_count øker med 1 for hver prosent batteriet øker.
    display.print(your_total_payment);
    display.print(" Kr");
    display.display();

    delay(1000);
  }
  bank_balance = bank_balance - your_total_payment;
}


void full_charge() // Når kunden velger dette, vil batteriet bli fulladet.
{
  people_charging(); // Kaller på hvor mange som lader, denne faktoren kommer til å påvirke hvor lang tid det tar for batteriet å lade.

  for (int i = batterycharge; i < 100; i++) // Leser av batteriverdien på bilen og øker opp til 100%
    {
      if (price_count * charging_price >= (bank_balance - charging_price)) // Dersom den totale belastningen av ladingen blir høyere enn kunden har på konto - hva den nåværende ladeprisen er, avbrytes ladingen.
      {
        break;
      }
      display_showing_charge();  // Displayet viser mengde lading i %.
    }
  if (batterycharge == 100) // Dersom batteriprosenten er 100, så vil funksjonen med fulladet batteri vises.
  {
    display_fully_charged();
  }
  else // Dersom batteriet er under 100% betyr det at saldoen til kunden er under 0, da vil funksjonen under kjøre.
  {
    for (int i = 0; i < 5; i++)
    {
      display.clearDisplay();
      
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0, 10);

      display.println("Charging stopped due to unsufficient amount of money");
      display.println(" ");
      display.print("Your current charge is: ");
      display.print(batterycharge);
      display.print(" % ");
      display.display();

      delay(1000);
    }
  }
  your_total_is();
}


void stop_charge_function() // Ladefunksjon som kjører til kunden stopper den, eller til den når 100%
{
  people_charging();
  
  // Må få en funksjon som leser av når man stopper ladingen
  if (stop_charge == false)
  {
    for (int i = batterycharge; i < 100; i++) // Øker oppp til 100%
    {
      if (price_count * charging_price >= bank_balance) // Dersom prisen på ladingen er større saldoen til kunden, avbrytes ladingen
      { 
        break;
      }
      display_showing_charge(); // Øker ladingen intill stoppknappen trykkes, eller prisen > saldoen.
           
       if (stop_charge == true); // Dersom stop_charge blir aktivert så stopper funksjonen
      {
        break;
      } 
    }
  }
  if ((batterycharge < 100) && (bank_balance >= charging_price)) // Hvis det stoppes før batteriprosenten når 100%
  {
    for (int i = 0; i < 5; i++)
    { 
      display.clearDisplay();
      
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0, 10);

      display.println("Charging stopped.");
      display.println(" ");
      display.print("Your current charge is: ");
      display.print(batterycharge);
      display.print(" % ");
      display.display();

      delay(1000);
    }  
  }
  else if ((batterycharge < 100) && (bank_balance < charging_price)) // Hvis ladingen avbrytes siden kunden ikke har mer penger på kontoen.
  {
    for (int i = 0; i < 5; i++)
    { 
      display.clearDisplay();
      
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0, 10);

      display.println("Charging stopped due to unsufficient amount of money");
      display.println(" ");
      display.print("Your current charge is: ");
      display.print(batterycharge);
      display.print(" % ");
      display.display();

      delay(1000);
    }
  }    
  else // Dersom det ikke stoppes før bilen når 100%
  {
    display_fully_charged();  
  }
  your_total_is();
}


void not_enough_money_charge() // Dersom kunden ikke har nok penger på saldoen, men fortsatt må lade så velger man denne.
{
  people_charging();
  if (bank_balance <= 0) // Lader dersom man har negativ saldo.
  {
    for (int i = batterycharge; i < 100; i++) // Fullader
    {
      display_showing_charge();
    }
    display_fully_charged();
    your_total_is();
  }
  if (bank_balance > 0) // Det skal ikke være mulig å velge denne lademuligheten dersom man har positiv saldo.
  {  
    for (int i = 0; i < 5; i++)
    {
      display.clearDisplay();
        
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0, 10);
  
      display.println("You have enough money, please choose a different charging method");
      display.display();

      delay(1000);
    }  
  }
}

void percentage_charge() // Lader opp til en viss prosent som kunden angir
{
  for (int i = batterycharge; i < percentage_value; i++)
  {
     if (batterycharge = 100) // Dersom batteriprosenten når 100% kjører denne
    {
      break;
    }
    display_showing_charge();
  }

   if (batterycharge = 100) // Dersom batteriprosenten når 100% kjører denne
  {
    display_fully_charged();
  }
  else
  {
    for (int i = 0; i < 5; i++) // Dersom det stoppes før bilen blir fulladet
    {
      display.clearDisplay();
        
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0, 10);
  
      display.println("Your current charge is: ");
      display.print(batterycharge);
      display.print("%");
      display.display();

      delay(1000);
    } 
  }
  your_total_is();
}


void timed_charge() // Denne funksjonen lader på en bestemt tid som kunden angir
{
  // Må ha en funksjon som leser av time_input
  
  for (int i = batterycharge; i < time_input; i++)
  { 
    if (batterycharge = 100) // Avbryter dersom batteriprosenten når 100%
    {
      break;
    }  
    display_showing_charge();   
  }

  if (batterycharge = 100) // Om den når 100% så viser funksjonen at bilen er fulladet.
  {
    display_fully_charged();
  }
  else
  {
    for (int i = 0; i < 5; i++) // Om bilen ikke når 100% kjører denne funksjonen, og viser den batteriprosenten den har nå
    {
      display.clearDisplay();
        
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0, 10);
  
      display.println("Your current charge is: ");
      display.print(batterycharge);
      display.print("%");
      display.display();

      delay(1000);
    } 
  }
  your_total_is();
}


void setup() 
{
  Serial.begin(115200);

  pinMode(choose_charge, INPUT);
  pinMode(activate_charge, INPUT);
  pinMode(stop_charge, INPUT);

  time_now = millis();

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) 
  {
    Serial.println("SSD1306 allocation failed");
    for(;;);
  }
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}



void loop() 
{
  if (!client.connected()) 
  {
    reconnect();
  }
  client.loop();
  
  repair_charging_station = random(0, 100000);

  charging_price = random(15,25);

  display.clearDisplay();

  batterycharge = random(70, 85);
  bank_balance = 800;
      
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);

  display.println("Group 27's charging ");
  display.println("station.");
  display.display();


  if (repair_charging_station <= 5) // 0,005% sannsynlighet for at ladestasjonen er ute av drift
  {
    time_out();
  }
  
  // Må skrive inn en kode her for å lese av activate_charge, enten det blir fra mobil eller knapp på zumo eller noe annet.
  activate_charge = true;
  if (activate_charge == true)
  {    
    choose_charge == 0;
    
    display_bank_balance();
    
    while (choose_charge == 0) // Forblir i denne while løkken intill det blir valgt en lading
    {
      // Må skrive inn en kode her for å lese av choose_charge, enten det blir fra mobil eller knapp på zumo eller noe annet.
      
      time_out_while_time = millis();
      
      display.clearDisplay();
      
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0, 10);

      display.println("Choose your charge.");
      display.println("Current charge price: ");
      display.print(charging_price);
      display.print(" Kr per kWh.");
      display.display();

      if (twenty_sec + previous_time_out_while_time <= time_out_while_time) // Dersom man ikke velger noe innen tjue sekunder så går man ut av while loopen
      {
        break;
      }
    } 
    previous_time_out_while_time = time_out_while_time; 

    // Finner ut hvilken lading som blir valgt

    choose_charge = 1;
    if (choose_charge == 1)
    {
      full_charge();
    }
    else if (choose_charge == 2)
    {
      percentage_charge();
    }
    else if (choose_charge == 3)
    {
      stop_charge_function();
    }
    else if (choose_charge == 4)
    {
      timed_charge();
    }
    else if (choose_charge == 5)
    {
      not_enough_money_charge();
    }   
    else
    {
      Serial.println("Have a good day!");
    }
  } 
  client.publish("esp3/battery", batterycharge);
  client.publish("esp3/balance", bank_balance);
  
  reset_variables();
}
