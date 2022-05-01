#include <Wire.h>
#include <Zumo32U4.h>

Zumo32U4Motors motors;
Zumo32U4ButtonA buttonA;
Zumo32U4LCD display;
Zumo32U4LineSensors lineSensors;
Zumo32U4Buzzer buzzer;
Zumo32U4Encoders encoders;

#define NUM_SENSORS 5
unsigned int lineSensorValues[NUM_SENSORS];

int position;
int16_t lastError = 0;

const uint16_t sensorThreshold = 200; //Brukt for å se når en sensor oppdager en linje
uint16_t sensorVals[5]; //Leser sensorverdiene for 5 sensorer



int batteryPercentage = 10;
int money = 100;
bool needCharge = false;
bool needsMoney = false;
float distance;




// Funksjon for speedometer
float speedometer(){
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
    distance += (average*3.14*0.078);
  
    // Viser hastighet på LCD
    char speedString = (char(nowSpeed) + "m/s");
    display.clear();
    display.gotoXY(0, 0);
    display.print(nowSpeed);
    display.gotoXY(0, 1);
    display.print(distance); 
  }
}




bool chargeCheck(){
  if(batteryPercentage < 20){
    needCharge = true;
  }
  else{
    needCharge = false;
  }
  return needCharge;
}








void setup() {

  lineSensors.initFiveSensors();  //Initierer alle 5 linjefølersensorene, initThreeSensors for bare de 3 midterste

  buttonA.waitForButton();  //Venter til knapp A trykket
  delay(500);              //Venter til knappetrykker har fjernet pølsefingrene
  calibrateSensors();
}




void loop() {
  position = lineSensors.readLine(lineSensorValues);

  lineFollow();
  speedometer();
}




void calibrateSensors()
{
  //Vent ett sekund for å få tid til å fjerne pølsefingrene fra knappen
  delay(1000);
  for(uint16_t i = 0; i < 120; i++)
  {
    if (i > 30 && i <= 90)
    {
      motors.setSpeeds(-200, 200);
    }
    else
    {
      motors.setSpeeds(200, -200);
    }
    lineSensors.calibrate();  //Benytter innebygd kalibreringsfunksjon
  }
  motors.setSpeeds(0, 0); //Stopper kjøring da kalibreringen er ferdig
}




void placeholdCharge(){
  if(needCharge){
    motors.setSpeeds(0, 0);
    delay(2000);
    batteryPercentage = 100;
    needCharge = false;
  }
}



bool aboveDark(uint8_t sensor)
{
  return sensorVals[sensor] > sensorThreshold;
}


bool aboveDarkSpot()  //Dersom alle sensorene er aktiverte er bilen over et svart område
{
  return aboveDark(0) && aboveDark(1) && aboveDark(2) && aboveDark(3) && aboveDark(4);
}


bool chargerToRight() //Dersom bare sensorene i midten og til høyre er aktivert er det en lader til høyre
{
  return !aboveDark(0) && aboveDark(1) && aboveDark(2) && aboveDark(3) && aboveDark(4);
}


uint16_t readSensors()  //Nødvendig for å lagre sensoravlesningene til å bli lest senere av aboveDark()
{
  return lineSensors.readLine(sensorVals);
}





void lineFollow(){

  int16_t error = position - 2000; //Feilmarginen er hvor langt unna senterlinjen (2000) vi er.

  int16_t spdDiff = error / 4 + (error - lastError);
  lastError = error;
  
  int spd = 200;  //Topphastighet er 400
  int16_t speedR = spd - spdDiff; //Ex. bil langt til høyre (spd - (-2000)) = 400
  int16_t speedL = spd + spdDiff; //Ex. bil langt til høyre (spd + (-2000)) = 0

  motors.setSpeeds(speedL, speedR); //setSpeeds(VENSTRE, HØYRE);

  readSensors();

  if(chargerToRight()){
    buzzer.playNote(NOTE_C(4),100,10);

    charge(speedL, speedR);
  }
}

void charge(speedL, speedR){
  if(chargeCheck()){
    motors.setSpeeds(200, 0);
    delay(500);               //Snu ca. 90 grader høyre
    while(!aboveDarkSpot()){  //Imens bilen ikke er ved ladestasjon
      motors.setSpeeds(speedL, speedR);
    }
    
    placeholdCharge();
    motors.setSpeeds(100, -100);
    delay(1500);                 //Snu ca. 180 grader høyre

    while(!aboveDarkSpot()){  //Imens bilen ikke er tilbake ved veien
      motors.setSpeeds(speedL, speedR);
    }

    motors.setSpeeds(200, 0);
    delay(500);
  }
}
