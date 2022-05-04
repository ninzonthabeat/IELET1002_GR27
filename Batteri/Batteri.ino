  /* Bankkonto
 *    - Trenger penger for å lade pluss service elns
 *    - Kommuniser banktransaksjoner 
 *    - Skal kunne hente ut transaksjoner og saldo 
 *    - Tjene penger
 *    
 * Softwarebatteri 
 *    - Visning av batteristatus
 *      - Ladningsnivå 0-100%
 *        - <10% lyd lys og display
 *        - <5% skal stoppe opp og bipe to ganger før den kjører videre hvert 15 sek
 *        - selve lading
 *          - Vis i lademodus og 15 sek etterpå
 *            -batteryLevel
 *            - charging_cost
 *            - account_balance
 *      - Batterihelse
 *        - Baseres på antall ladesykluser
 *        - antall ganger < 5%
 *        - de tre 60 sek variablene 
 *        - Bruk dette i en selvbestemt formel
 *        - Skal lagre i bilens EEPROM
 *        - Rand-produktfeil 
 *          - Skal redusere batterihelse med 50%
 *          
 *        - Under nivå 1 må den utføres service
 *        - Under nivå 0 må batteri byttes
 *        - Fikses med knappetrykk - koster penger
 *        - Bytte gir beste verdi, service fikse x%
 */

#include <Wire.h>
#include <Zumo32U4.h>
#include <math.h>
#include <EEPROM.h>

Zumo32U4Encoders encoders;
Zumo32U4Motors motors;
Zumo32U4ButtonA buttonA;
Zumo32U4ButtonB buttonB;
Zumo32U4ButtonC buttonC;
Zumo32U4LCD display;


// Variabler for batteri, ladning og helse
// alle under oppdateres
int batteryLevel = 100;
int battery_health;
int charging_cycles;
int times_under_5;

float new_battery_level;
bool runState = true;

// Variabler for millis
float millisBegin1;
float millisBegin2;

// Variabler for avstand og fart
float total_distance;
// float total_time; Denne er bare millis()

// Variabler for økonomi
int charging_cost;
int cost_per = 10; // Hent denne prisen, den er nok variabel 


// målinger for de siste 60 sek, de lagres som globale verdier som settes til null hvert 60 sek
// deklarerer også midlertidige variabler
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

/////////////////////////////////////////////////////////////////////

// Tror det er lurt med en egen funksjon som tar seg av display mtp hvor mye rart som skal opp der
void setup() {
  Serial.begin(9600);

  // Hent verdier fra EEPROM når man starter opp zumo 
  battery_health = EEPROM.read(addr_health);
  times_under_5 = EEPROM.read(addr_5);
}

void loop() {
  // Sjekke om det er lite strøm eller evt er tomt
  check_battery_level();
  
  // Om det er tomt for strøm går ikke bilen
  if(runState == false || battery_health == 0){
    motors.setSpeeds(0,0);
  } else{
    motors.setSpeeds(100,100);
  }

  // Spørring som undersøker om det skal lades på noen måte
  // også om det skal utføres service 
  if (buttonB.getSingleDebouncedPress()){
    hidden_charge();
  } else if(buttonC.getSingleDebouncedPress()){
    emerg_charge();
  } else if(buttonA.getSingleDebouncedPress()){
    service_pressed();
  }
  
  // Kjører speedometer-funksjon
  // speedometer();


  // Legg til en "if charge button press" 
  // Skal kjøre en funksjon "charge_state"
  // Leg til charge cyclus +1 
  // Må finne ut hvordan lading skal funke med Patrick og Nina
  // charge_state();

  
  // Starter en millis
  float millisNow = millis();

  // Millis som skal sjekke tilstander hvert sekund
  if(millisNow-millisBegin1 >= 1000){
    track_variable();

    display.clear();

    find_battery_level();

    display.gotoXY(0,0);
    display.print("h = ");
    display.print(battery_health);

    
    millisBegin1 = millisNow;
  }

  // Ny millis for å bestemme og nullstille verdier
  if(millisNow-millisBegin2 >= 60000){
    find_variable_end();
    
    millisBegin2 = millisNow;
 }

  // Prøve å lage en egen display funksjon elns
}

////////////////////////////////////////////////////////////////////////////

// Funksjon som tar seg av hva som blir displayet

// Funksjon som henter pris for strøm nå
int current_price(){

}

// Funksjon som henter saldo // Trenger også en måte å oppdatere saldo 
int account_balance(){

}

// Funksjon som finner hastighet
float find_speed(){
  // Henter rotasjoner 
  int16_t countsLeft = encoders.getCountsAndResetLeft();
  int16_t countsRight = encoders.getCountsAndResetRight();

  // Regner ut avstand
  float average = ((countsLeft/909.7)+(countsRight/909.7))/2;
  float nowSpeed = (average *3.14*(0.078))/0.1;

  // Evt bare del counts på 7700, tror d skal gi m/s

  return nowSpeed;
}


// Funksjon for speedometer
void speedometer(){
  static uint8_t lastDisplayTime;

  if ((uint8_t)(millis() - lastDisplayTime) >= 100){

    lastDisplayTime = millis();
      
    // Henter rotasjoner 
    int16_t countsLeft = encoders.getCountsAndResetLeft();
    int16_t countsRight = encoders.getCountsAndResetRight();
  
    // Regner ut avstand
    float average = ((countsLeft/909.7)+(countsRight/909.7))/2;
    float nowSpeed = (average *3.14*(0.078))/0.1;

    // Legger til på avstand
    total_distance += abs((average*3.14*0.078));
  
    // Viser hastighet på lcd
    char speedString = ("v = " + char(nowSpeed));
    char distanceString = ("s =" + char(total_distance));
    
    display.clear();
    display.gotoXY(0,0);
    display.print("v=");
    display.print(nowSpeed);
    display.gotoXY(0, 1);
    display.print("s=");
    display.print(total_distance);
  }
}


// Funksjon som finner gjennomsnitt av hastighet, maks hastighet og tid over 70% av
// absolutt maks hastighet
void track_variable(){
    // Henter farten
    float speed_now = find_speed();

    // Dersom hastighet over ca. 70% av maks legges det til 1 sek
    if(speed_now >= 0.80){
      sec_over_70_current += 1;
    }

    // Finner maks hastighet
    if(speed_now > max_speed_current){
      max_speed_current = speed_now;
    }

    // Finne gjennomsnittet
    average_speed_num += speed_now;
    n += 1;   
}


// Funksjon som finner maks hastighet, tid over 70% og gjennomsnittlig hastighet
// for de siste 60 sek
float find_variable_end(){
   // setter bestemmer de tre variablene
    max_speed = max_speed_current;
    sec_over_70 = sec_over_70_current;
    average_speed = average_speed_num/n;

    // Nullstiller
    max_speed_current = 0;
    sec_over_70_current = 0;
    average_speed_num = 0;
    n = 0;

    //millisBegin2 = millisNow; // Tror ikke denne funker
}


// Funksjon for sw-batteriet,
float find_battery_level(){
  // Henter hastighet
  float speed_now = find_speed();
  
  // Funksjon som finner ny batteriverdi som funksjon av hastighet
  float previous_battery_level = batteryLevel;
  batteryLevel = previous_battery_level-(speed_now)*0.90;

  display.gotoXY(0,1);
  display.print("% = ");
  display.print(batteryLevel);

  return new_battery_level;
}


// Funksjon som sjekker batterinivå og gir alarmer ved laver nivå enn 10 og 5 prosent
void check_battery_level(){
  
  // Om batterinivå er under 10%
  if(batteryLevel > 5 && batteryLevel <= 10){
    // legg til blink eller alarm elns
  }
  // Om batterinivå er under 5%
  if(batteryLevel <= 5){
    // Legg til blink og alarm og stopp og shit
  }
  // Om batteriet er flatt
  if (batteryLevel == 0){
    runState = false;
  }
}


// Funksjon for batteri-helse 
float get_battery_health(){
  float previous_battery_health = battery_health;

  // Ikke ferdig med denne formelen men dgb
  battery_health = previous_battery_health - (sec_over_70+max_speed+average_speed)/(100)-(charging_cycles+times_under_5)/(100);

  // Lagrer til EEPROM 
  EEPROM.write(addr_5, times_under_5);
  EEPROM.write(addr_health, battery_health);
}


// Funksjon for en hemmelig lademodus
void hidden_charge(){
  motors.setSpeeds(-50,-50);
  delay(5000);
  batteryLevel += 10;
  charging_cycles += 1;
}


// Funksjon for nødladning, 
void emerg_charge(){
  motors.setSpeeds(-50,-50);
  delay(5000);
  batteryLevel += 20;
  charging_cycles += 1;  
}


// Funksjon for service ell. bytte av batteri 
void service_pressed(){
  // Lager en spørring som sjekker hvor mye prosent man har
  if (battery_health >= 90){
    // Cervice men kan ikke ha over 100% 
    battery_health = 100;
    // NB trekk en pengesum
  } else if (battery_health >= 20){
    // Service
    battery_health += 10;
  } else if (battery_health < 20){
    // Bytter batteri
    battery_health = 100;
  }
}
