#include "scheduler.h"
#include <Servo.h>
int command[2];
int indx = 0;
int LZ = A0, PAN = 53, TILT = 52;
int panValue = 2050;
int tiltValue = 950;
int maxAdjustTilt = 10;
int maxAdjustPan = 40;
int speedTilt = 0;
int oldTiltSpeed = 0;
int speedPan = 0;
int oldPanSpeed = 0;
Servo panServo;
Servo tiltServo;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial1.begin(9600);
  pinMode(LZ,OUTPUT);
  panServo.attach(PAN);
  tiltServo.attach(TILT);
  analogWrite(LZ,0);
  Scheduler_Init();
 
  // Start task arguments are:
  //    start offset in ms, period in ms, function callback
 
  Scheduler_StartTask(0, 10, readSerial1);
  Scheduler_StartTask(2, 40, tiltHandler);
  Scheduler_StartTask(4, 40, panHandler);
}
// handling tilt
void tiltHandler(){
  int tempT = speedTilt;
  if(oldTiltSpeed > tempT){
    tempT = oldTiltSpeed - 1;
  }
  else if(oldTiltSpeed < tempT){
    tempT = oldTiltSpeed + 1;
  }
  if(tiltValue + tempT > 1000 || tiltValue + tempT < 500){
    tempT = 0;
  }
      
  oldTiltSpeed = tempT;
  if(tempT != 0) {
    tiltValue += tempT;
    if(tiltValue >= 1000)
      tiltValue = 1000;
    if(tiltValue <= 500)
      tiltValue = 500;
    tiltServo.writeMicroseconds(tiltValue);
  }
  
}
//pan handling
void panHandler(){
  int tempP = speedPan;
  if(oldPanSpeed - tempP > 2){
    tempP = oldPanSpeed - 2;
  }
  else if( oldPanSpeed - tempP < -2){
    tempP = oldPanSpeed + 2;
  }
  if(panValue + tempP > 2100 || panValue + tempP < 750){
    tempP = 0;
  }
  oldPanSpeed = tempP;
  if(tempP != 0) {
    panValue += tempP;
    if(panValue >= 2100)
      panValue = 2100;
    if(panValue <= 750)
      panValue = 750;
    panServo.writeMicroseconds(panValue);
  }
    
}

void callFunction(){
  int v0 = command[0],v1 = command[1];
  switch(v0){
    case 1:
      switch(v1){
        case 1:
          analogWrite(LZ,200);
          break;
        case 2:
          analogWrite(LZ,0);
          break;
        case 3 ... 84:
          speedPan = map(v1, 3, 84, -maxAdjustPan, maxAdjustPan);
          break;
        default: return;
      }
      break;
     case 2:
     switch(v1){
     case 1 ... 22:
          speedTilt = map(v1, 1, 22, -maxAdjustTilt, maxAdjustTilt);
          break;
          default: return;
     }
     break;
     default: return;
    
  }
   //Serial.write(v0);
   //Serial.write(v1);
}

void readSerial1(){
  // put your main code here, to run repeatedly:
  if(Serial1.available()){
    int readVal = Serial1.read();
    Serial.println(readVal);
    if(readVal == 0) {
      indx = 0;
      return;
    }
    command[indx++] = readVal;
    if(indx > 1){
      callFunction();
      indx = 0;
    }
  }
}

// idle task
void idle(uint32_t idle_period)
{
  // this function can perform some low-priority task while the scheduler has nothing to run.
  // It should return before the idle period (measured in ms) has expired.  For example, it
  // could sleep or respond to I/O.
 
  readSerial1();
  delay(idle_period);
}

void loop() {

  uint32_t idle_period = Scheduler_Dispatch();
  if (idle_period)
  {
    idle(idle_period);
  }
}
