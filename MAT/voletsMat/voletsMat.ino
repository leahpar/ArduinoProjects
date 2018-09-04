// --- IR ---------------------------------------------

#include <IRremote.h>
//#include <IRrecv.h>
//#include <IRutils.h>

const int receivePin = 2;
IRrecv irrecv(receivePin);
decode_results results;

const int minimumPressTime = 100; // ms to keep the key pressed for

const int PIN_ON_OFF = 3;
const int PIN_UP_DOWN = 4;
const int DELAY_RELAY = 150;

bool repeatable = false;
unsigned long pressTime = 0;
bool pressingKeys = false;

void setup() {
  Serial.begin(74880);
  while (!Serial);
  Serial.println("Start");

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_ON_OFF, OUTPUT);
  pinMode(PIN_UP_DOWN, OUTPUT);

  // --- IR ---------------------------------------------
  
  Serial.println("Start the IR receiver...");
  irrecv.enableIRIn();

  Serial.println("Init relay down...");
  digitalWrite(PIN_ON_OFF, LOW);
  digitalWrite(PIN_UP_DOWN, LOW);

  Serial.println("Done");
}

void blink(int up, int down) {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(up);
  digitalWrite(LED_BUILTIN, LOW);
  delay(down);
}



void loop() {
  if (irrecv.decode(&results)) {

    Serial.print("ir code : ");
    Serial.print(results.value, HEX);
    Serial.println("");
    
    switch (results.value) {
      case 0xFFFFFFFF: // Repeat code
        // Don't release the key unless it's a non repeatable key.
        if (repeatable) {
          pressTime = millis(); // Keep the timer running.
        }
        break;

      case 0x10ED609F: // UP (CH+)
      //case 0x10EDD827: // UP (VOL+)
        Serial.println("UP");
        repeatable = false;
        digitalWrite(PIN_ON_OFF, LOW);
        delay(DELAY_RELAY);
        digitalWrite(PIN_UP_DOWN, HIGH);
        delay(DELAY_RELAY);
        digitalWrite(PIN_ON_OFF, HIGH);
        // Show something
        blink(50, 10);
        break;

      case 0x10ED6897: // DOWN (CH-)
      //case 0x10ED5AA5: // UP (VOL-)
        Serial.println("DOWN");
        repeatable = false;
        digitalWrite(PIN_ON_OFF, LOW);
        delay(DELAY_RELAY);
        digitalWrite(PIN_UP_DOWN, LOW);
        delay(DELAY_RELAY);
        digitalWrite(PIN_ON_OFF, HIGH);
        // Show something
        blink(50, 10);
        break;

      case 0x10ED1AE5: // STOP (INFO)
      //case 0x10ED18E7: // STOP (SUBTITLE)
        Serial.println("STOP");
        repeatable = false;
        digitalWrite(PIN_ON_OFF, LOW);
        delay(DELAY_RELAY);
        digitalWrite(PIN_UP_DOWN, LOW);
        // Show something
        blink(50, 10);
        break;

      // Non-matching IR
      default:
        repeatable = false;
        break;
    }

    // Receive the next value
    delay(25);
    irrecv.resume();
  }
  else {
    unsigned long now = millis();
    if ((now - pressTime > minimumPressTime) && pressingKeys) {
      //releaseAll();
      pressingKeys = false;
    }
    // { else repeat ? }
  }
}

