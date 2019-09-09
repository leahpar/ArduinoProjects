
const int PIN_MOTOR_1 = 4;
const int PIN_MOTOR_2 = 3;
const int PIN_MOTOR_3 = 2;
const int PIN_MOTOR_4 = 5;

const int PIN_POWER = 6;
const int PIN_DIR   = 7;
const int PIN_SPEED = 8;
const int PIN_LASER = 9;

const int UP    = LOW;
const int DOWN  = HIGH;

const int FAST  = HIGH;
const int SLOW  = LOW;

const int ON    = HIGH;
const int OFF   = LOW;

const byte patterns[8] = {
  0b00001000,
  0b00001100,
  0b00000100,
  0b00000110,
  0b00000010,
  0b00000011,
  0b00000001,
  0b00001001,
};

unsigned long t1 = 0, t2 = 0;

bool button_power, button_dir, button_speed, button_laser;

void setup() {

  Serial.begin(74880);
  while (!Serial);
  Serial.println("OK");
  
  pinMode(PIN_MOTOR_1, OUTPUT);
  pinMode(PIN_MOTOR_2, OUTPUT);
  pinMode(PIN_MOTOR_3, OUTPUT);
  pinMode(PIN_MOTOR_4, OUTPUT);

  pinMode(PIN_POWER, INPUT_PULLUP);
  pinMode(PIN_DIR,   INPUT_PULLUP);
  pinMode(PIN_SPEED, INPUT_PULLUP);
  pinMode(PIN_LASER, INPUT_PULLUP);
  
}

void loop() {
  //t2 = micros();
  //Serial.println(t2-t1);
  //delay(10);
  //t1 = micros();
  // Tps mesuré : 9592us

  readButtons();

  if (button_power == ON) {
    if (button_speed == SLOW) {
      step();
      delayMicroseconds(9317);
    }
    else {
      step2();
      delay(2);    
    }
  }
}


/*
 * https://forum.arduino.cc/index.php?topic=574391.msg3911823#msg3911823
 * 
 */
void step() {
  static int i = 0;
  digitalWrite(PIN_MOTOR_1, bitRead(patterns[i], button_dir == UP ? 0 : 3));
  digitalWrite(PIN_MOTOR_2, bitRead(patterns[i], button_dir == UP ? 1 : 2));
  digitalWrite(PIN_MOTOR_3, bitRead(patterns[i], button_dir == UP ? 2 : 1));
  digitalWrite(PIN_MOTOR_4, bitRead(patterns[i], button_dir == UP ? 3 : 0));
  i=++i%8;
}

void step2() {
  static int j = 0;
  digitalWrite(PIN_MOTOR_1, bitRead(patterns[j], button_dir == UP ? 0 : 3));
  digitalWrite(PIN_MOTOR_2, bitRead(patterns[j], button_dir == UP ? 1 : 2));
  digitalWrite(PIN_MOTOR_3, bitRead(patterns[j], button_dir == UP ? 2 : 1));
  digitalWrite(PIN_MOTOR_4, bitRead(patterns[j], button_dir == UP ? 3 : 0));
  j=(j+2)%8;
}

void readButtons() {
  button_power = digitalRead(PIN_POWER);
  button_dir   = digitalRead(PIN_DIR);
  button_speed = digitalRead(PIN_SPEED);
  button_laser = digitalRead(PIN_LASER);
}
