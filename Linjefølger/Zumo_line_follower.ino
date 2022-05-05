#include <Wire.h>
#include <Zumo32U4.h>
#include "TurnSensor.h"

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
  float nowSpeed;

  if ((uint8_t)(millis() - lastDisplayTime) >= 100){

    lastDisplayTime = millis();
      
    // Henter rotasjoner 
    int16_t countsLeft = encoders.getCountsAndResetLeft();
    int16_t countsRight = encoders.getCountsAndResetRight();
  
    // Regner ut avstand
    float average = ((countsLeft/909.7)+(countsRight/909.7))/2;
    nowSpeed = (average *3.14*(0.078))/0.1;

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

  return nowSpeed;
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
  
  Serial1.begin(115200); //Begynner Serial1 med baud-rate til ESP32

  lineSensors.initFiveSensors();  //Initierer alle 5 linjefølersensorene, initThreeSensors for bare de 3 midterste

  buttonA.waitForButton();  //Venter til knapp A trykket
  delay(500);              //Venter til knappetrykker har fjernet pølsefingrene
  calibrateSensors();
  turnSensorSetup();
}




void loop() {
  position = lineSensors.readLine(lineSensorValues);

  lineFollow();
  speedometer();

  readSensors();

  while(Serial1.available() > 0){
    Serial1.println(speedometer());
    /*
    int inbyte = Serial1.read();
    if(String(inbyte) == "update"){
      float message[] = {speedometer(), 50};
      Serial1.print(message);
    }*/
  }
  
  if(chargerToRight()){ //Dersom lader til høyre og trenger lading

    buzzer.playNote(NOTE_G(4),50,15);
    turn('R');
    
  }
  if(aboveDarkSpot() == true && needCharge == true){
    motors.setSpeeds(0,0);
    placeholdCharge();
    turn('U');
  } else if (aboveDarkSpot() == true && needCharge == false){
    turn('R');
  }
}



void turn(char dir){

  //Initierer startverdi for å lese vinkel i løpet av svingen
  int angle;
  
  switch(dir){
    
    case 'L':
      turnSensorReset();                                  //Resetter svingsensoren før sving
      motors.setSpeeds(-100, 100);
      angle = 0;
      do {                                                //Do-while utfører sjekk hver gang etter at loopen har kjørt 
        delay(1);
        turnSensorUpdate();                               //Oppdater sensor
        angle = (((int32_t)turnAngle >> 16) * 360) >> 16; //Oppdater avlest vinkel
      } while (angle < 90);                               //Imens bilen ikke har svingt 90 grader
      buzzer.playNote(NOTE_C(4),500,15);
      break;

    case 'R':
      turnSensorReset();
      motors.setSpeeds(100, -100);
      angle = 0;
      do {
        delay(1);
        turnSensorUpdate();
        angle = (((int32_t)turnAngle >> 16) * 360) >> 16;
      } while (angle > -90);
      buzzer.playNote(NOTE_C(4),500,15);
      break;

    case 'U':
      turnSensorReset();
      motors.setSpeeds(-100, 100);
      angle = 0;
      do {
        delay(1);
        turnSensorUpdate();
        angle = (((int32_t)turnAngle >> 16) * 360) >> 16;
      } while (angle < 170);                              //ved 180 grader vil den fortsette å rotere
      buzzer.playNote(NOTE_C(4),500,15);
      break;
  }
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
  if(aboveDark(0) == true && aboveDark(1) == true && aboveDark(2) == true && aboveDark(3) == true && aboveDark(4) == true){
    return true;
  } else {
    return false;
  }
  //return aboveDark(0) && aboveDark(1) && aboveDark(2) && aboveDark(3) && aboveDark(4);
}


bool chargerToRight() //Dersom bare sensorene i midten og til høyre er aktivert er det en lader til høyre
{
  if(chargeCheck()){
    return !aboveDark(0) && aboveDark(1) && aboveDark(2) && aboveDark(3) && aboveDark(4);
  }
}


uint16_t readSensors()  //Nødvendig for å lagre sensoravlesningene til å bli lest senere av aboveDark()
{
  return lineSensors.readLine(sensorVals);
}





void lineFollow(){

  int16_t error = position - 2000; //Feilmarginen er hvor langt unna senterlinjen (2000) vi er.

  int16_t spdDiff = error / 4 + (error - lastError);
  lastError = error;
  
  int spd = 250;  //Topphastighet er 400
  int16_t speedR = spd - spdDiff; //Ex. bil langt til høyre (spd - (-2000)) = 400
  int16_t speedL = spd + spdDiff; //Ex. bil langt til høyre (spd + (-2000)) = 0

  motors.setSpeeds(speedL, speedR); //setSpeeds(VENSTRE, HØYRE);
/*
  readSensors();

  if(chargerToRight()){
    buzzer.playNote(NOTE_C(4),100,15);

    charge();
  }*/
}



void charge(){

  
  if(chargeCheck()){
    motors.setSpeeds(200, 0);
    delay(500);               //Snu ca. 90 grader høyre

    while(!aboveDarkSpot()){  //Imens bilen ikke er ved ladestasjon
      lineFollow();
    }
    
    placeholdCharge();
    motors.setSpeeds(100, -100);
    delay(1500);                 //Snu ca. 180 grader høyre

    while(!aboveDarkSpot()){  //Imens bilen ikke er tilbake ved veien
      lineFollow();
    }

    motors.setSpeeds(200, 0);
    delay(500);
    lineFollow();
  }
}
