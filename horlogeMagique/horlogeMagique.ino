
/*
 Stepper Motor Control - one revolution

 This program drives a unipolar or bipolar stepper motor.
 The motor is attached to digital pins 8 - 11 of the Arduino.

 The motor should revolve one revolution in one direction, then
 one revolution in the other direction.


 Created 11 Mar. 2007
 Modified 30 Nov. 2009
 by Tom Igoe

 */

int motorPin1 = 5;
int motorPin2 = 4;
int motorPin3 = 3;
int motorPin4 = 2;
                       
int motorSpeed = 0;
const int maxMotorSpeed = 20;
int stepDelay = 0;

const int actionDelay = 50;

#include <IRremote.h>
const int receivePin = 6;
IRrecv irrecv(receivePin);
decode_results results;

unsigned long chrono;

enum MachineStates {OFF, CLOCKWISE, COUNTERCLOCKWISE};
enum MachineActions {NONE, PLUS, MINUS, STOP, INVERT, GOPLUS, GOMOINS};
MachineStates machineState = OFF;
MachineActions machineAction = NONE;
MachineActions lastAction    = NONE;

void setup() {
  Serial.begin(74880);

  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(motorPin3, OUTPUT);
  pinMode(motorPin4, OUTPUT);
 
  irrecv.enableIRIn();

  chrono = millis();

  Serial.println("OK");
}


void loop() {
  if (irrecv.decode(&results)) {
    /*
    Serial.print("ir code : ");
    Serial.print(results.value, HEX);
    Serial.println("");
    */
    switch (results.value) {
      case 0xFFFFFFFF: // repeat
        if (millis() - chrono > actionDelay) {
          chrono = millis();
          machineAction = lastAction;
        }
        break;

      case 0x77E1606B: // GOPLUS
        machineAction = GOPLUS;
        lastAction = NONE; // don't repeat
        break;

      case 0x77E1906B: // GOMOINS
        machineAction = GOMOINS;
        lastAction = NONE; // don't repeat
        break;

      case 0x77E1A06B: // STOP
        machineAction = STOP;
        lastAction = NONE; // don't repeat
        break;

      case 0x77E1506B: // PLUS
        chrono = millis();
        machineAction = PLUS;
        lastAction = PLUS;
        break;

      case 0x77E1306B: // MOINS
        chrono = millis();
        machineAction = MINUS;
        lastAction = MINUS;
        break;

      case 0x77E1C06B: // CHANGE DIR
        machineAction = INVERT;
        lastAction = NONE; // don't repeat
        break;

      case 0x7E1E06B:  // keyup
        switch (lastAction) {
          case PLUS:
          case MINUS:
            machineAction = NONE;
            break;
        }
        break;

}

    irrecv.resume();
  }

  doMachine();
}

void incMotorSpeed(int inc) {
  motorSpeed += inc;
  if (motorSpeed <= 0) {
    motorSpeed = 0;
    setMachineState(OFF, __LINE__);
    machineAction = NONE;
    lastAction = NONE;
  }
  else if (motorSpeed > maxMotorSpeed) {
    motorSpeed = maxMotorSpeed;
    machineAction = NONE;
    lastAction = NONE;
  }
  else {
    stepDelay = maxMotorSpeed / motorSpeed;
  }
  Serial.println("Speed: " + String(motorSpeed));
}

void doMachine() {
  switch (machineAction) {
    case PLUS:
      switch (machineState) {
        case OFF:
          incMotorSpeed(1);
          setMachineState(CLOCKWISE, __LINE__);
          break;
        case CLOCKWISE:
          incMotorSpeed(1);
          break;
        case COUNTERCLOCKWISE:
          incMotorSpeed(-1);          
          break;
      }
      machineAction = NONE;
      break;
    case MINUS:
      switch (machineState) {
        case OFF:
          incMotorSpeed(1);
          setMachineState(COUNTERCLOCKWISE, __LINE__);
          break;
        case CLOCKWISE:
          incMotorSpeed(-1);
          break;
        case COUNTERCLOCKWISE:
          incMotorSpeed(1);
          break;
      }
      machineAction = NONE;
      break;
    case STOP:
      if (millis() - chrono > actionDelay) {
        chrono = millis();
        incMotorSpeed(-1);
        if (motorSpeed > 15) incMotorSpeed(-1);
        if (motorSpeed > 8) incMotorSpeed(-1);
      }
      break;
    case GOPLUS:
    case GOMOINS:
      if (machineState == OFF && machineAction == GOPLUS) {
        setMachineState(CLOCKWISE, __LINE__);
      }
      else if (machineState == OFF && machineAction == GOMOINS) {
        setMachineState(COUNTERCLOCKWISE, __LINE__);
      }
      if (millis() - chrono > actionDelay) {
        chrono = millis();
        incMotorSpeed(1);
        if (motorSpeed > 15) incMotorSpeed(1);
        if (motorSpeed > 8) incMotorSpeed(1);
      }
      break;
    case INVERT:
      switch (machineState) {
        case CLOCKWISE:
          setMachineState(COUNTERCLOCKWISE, __LINE__);
          break;
        case COUNTERCLOCKWISE:
          setMachineState(CLOCKWISE, __LINE__);
          break;
      }
      machineAction = NONE;
      break;
  }
 
  switch (machineState) {
    case CLOCKWISE:
      motor_clockwise();
      break;
    case COUNTERCLOCKWISE:
      motor_counterclockwise();
      break;
  }
}


//==============================================================================
// SET NEW MACHINE STATE
//==============================================================================
void setMachineState(MachineStates state, int line) {
  Serial.println(
      String("[")
    + String((int)(millis()/1000))
    + "s;l." + String(line)
    + "] "
    + getMachineStateString(machineState)
    + " => "
    + getMachineStateString(state)
  );
  machineState = state;
}

//==============================================================================
// GET MACHINE STATE IN STRING
//==============================================================================
String getMachineStateString(MachineStates state) {
  switch (state) {
    case OFF:           return String("OFF");
    case CLOCKWISE:    return String("CLOCKWISE");
    case COUNTERCLOCKWISE:    return String("COUNTERCLOCKWISE");
    case INVERT:         return String("INVERT");
  }
}

//////////////////////////////////////////////////////////////////////////////
//set pins to ULN2003 high in sequence from 1 to 4
//delay "motorSpeed" between each pin setting (to determine speed)

void motor_counterclockwise (){
 // 1
 digitalWrite(motorPin1, HIGH);
 digitalWrite(motorPin2, LOW);
 digitalWrite(motorPin3, LOW);
 digitalWrite(motorPin4, LOW);
 delay(stepDelay);
 // 2
 digitalWrite(motorPin1, HIGH);
 digitalWrite(motorPin2, HIGH);
 digitalWrite(motorPin3, LOW);
 digitalWrite(motorPin4, LOW);
 delay (stepDelay);
 // 3
 digitalWrite(motorPin1, LOW);
 digitalWrite(motorPin2, HIGH);
 digitalWrite(motorPin3, LOW);
 digitalWrite(motorPin4, LOW);
 delay(stepDelay);
 // 4
 digitalWrite(motorPin1, LOW);
 digitalWrite(motorPin2, HIGH);
 digitalWrite(motorPin3, HIGH);
 digitalWrite(motorPin4, LOW);
 delay(stepDelay);
 // 5
 digitalWrite(motorPin1, LOW);
 digitalWrite(motorPin2, LOW);
 digitalWrite(motorPin3, HIGH);
 digitalWrite(motorPin4, LOW);
 delay(stepDelay);
 // 6
 digitalWrite(motorPin1, LOW);
 digitalWrite(motorPin2, LOW);
 digitalWrite(motorPin3, HIGH);
 digitalWrite(motorPin4, HIGH);
 delay (stepDelay);
 // 7
 digitalWrite(motorPin1, LOW);
 digitalWrite(motorPin2, LOW);
 digitalWrite(motorPin3, LOW);
 digitalWrite(motorPin4, HIGH);
 delay(stepDelay);
 // 8
 digitalWrite(motorPin1, HIGH);
 digitalWrite(motorPin2, LOW);
 digitalWrite(motorPin3, LOW);
 digitalWrite(motorPin4, HIGH);
 delay(stepDelay);
}

//////////////////////////////////////////////////////////////////////////////
//set pins to ULN2003 high in sequence from 4 to 1
//delay "motorSpeed" between each pin setting (to determine speed)

void motor_clockwise(){
  // 1
 digitalWrite(motorPin4, HIGH);
 digitalWrite(motorPin3, LOW);
 digitalWrite(motorPin2, LOW);
 digitalWrite(motorPin1, LOW);
 delay(stepDelay);
 // 2
 digitalWrite(motorPin4, HIGH);
 digitalWrite(motorPin3, HIGH);
 digitalWrite(motorPin2, LOW);
 digitalWrite(motorPin1, LOW);
 delay (stepDelay);
 // 3
 digitalWrite(motorPin4, LOW);
 digitalWrite(motorPin3, HIGH);
 digitalWrite(motorPin2, LOW);
 digitalWrite(motorPin1, LOW);
 delay(stepDelay);
 // 4
 digitalWrite(motorPin4, LOW);
 digitalWrite(motorPin3, HIGH);
 digitalWrite(motorPin2, HIGH);
 digitalWrite(motorPin1, LOW);
 delay(stepDelay);
 // 5
 digitalWrite(motorPin4, LOW);
 digitalWrite(motorPin3, LOW);
 digitalWrite(motorPin2, HIGH);
 digitalWrite(motorPin1, LOW);
 delay(stepDelay);
 // 6
 digitalWrite(motorPin4, LOW);
 digitalWrite(motorPin3, LOW);
 digitalWrite(motorPin2, HIGH);
 digitalWrite(motorPin1, HIGH);
 delay (stepDelay);
 // 7
 digitalWrite(motorPin4, LOW);
 digitalWrite(motorPin3, LOW);
 digitalWrite(motorPin2, LOW);
 digitalWrite(motorPin1, HIGH);
 delay(stepDelay);
 // 8
 digitalWrite(motorPin4, HIGH);
 digitalWrite(motorPin3, LOW);
 digitalWrite(motorPin2, LOW);
 digitalWrite(motorPin1, HIGH);
 delay(stepDelay);
}


