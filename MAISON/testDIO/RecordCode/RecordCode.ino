/*
  RecordCode.ino - Program to acquire the DiO modules On/Off codes
  Created by Charles GRASSIN, July 31, 2017.
  Under MIT License (free to use, modify, redistribute, etc.).
  www.charleslabs.fr

  This example captures codes sent by DiO 433MHz remotes. To read
  the frames, plug a RX 433MHz module to the Arduino, to pin 2.
  Open serial monitor at 9600 bauds. Aim the remote at the module,
  and the code will be display in the monitor. Capture "On" and "Off"
  codes.
*/

// --- HomeEasy protocol parameters ---
#define DiOremote_PULSEIN_TIMEOUT 1000000
#define DiOremote_FRAME_LENGTH 64
// Homeasy start lock : 2725 us (+/- 175 us)
#define DiOremote_START_TLOW 2675-100
#define DiOremote_START_THIGH 2675+100

#define MARGE 30
// Homeeasy 0 : 310 us
#define DiOremote_0_TLOW 275-MARGE
#define DiOremote_0_THIGH 275+MARGE
//Homeeasy 1 : 1300 us
#define DiOremote_1_TLOW 1225-MARGE
#define DiOremote_1_THIGH 1225+MARGE

// --- Variables ---
const unsigned int RX_PIN = D2;  // 443 MHz rx module pin
unsigned long code = 0;         // Variable to store the code
int i    ;                      // Loop iteration variable
unsigned long t = 0;            // Time between two falling edges
byte currentBit,previousBit;

// --- Setup ---
void setup() {
  pinMode(RX_PIN, INPUT);
  Serial.begin(74880);
  while (!Serial);
  Serial.println("start");
}

// --- Loop ---
void loop() {
  //Serial.print("readRadio... ");
  // Wait for incoming bit
  t = pulseIn(RX_PIN, LOW, DiOremote_PULSEIN_TIMEOUT);
  //Serial.println(String(t) + "us");

  // Only decypher from 2nd Homeeasy lock
  if (t > DiOremote_START_TLOW && t < DiOremote_START_THIGH) {
Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXX");
code = 0;

    for (i = 0 ; i < DiOremote_FRAME_LENGTH ; i++) {
      // Read each bit (64 times)
      t = pulseIn(RX_PIN, LOW, DiOremote_PULSEIN_TIMEOUT);

      // Identify current bit based on the pulse length
      if (t > DiOremote_0_TLOW && t < DiOremote_0_THIGH) {
        Serial.print(0);
        currentBit = 0;
      } else if (t > DiOremote_1_TLOW && t < DiOremote_1_THIGH) {
        Serial.print(1);
        currentBit = 1;
      } else {
        Serial.print(".");
        i--;
        //continue;
        break;
      }
      // If bit count is even, check Manchester validity & store in code
      if (i % 2) {

        // Code validity verification (Manchester complience)
        if (!(previousBit ^ currentBit))
          break;

        // Store new bit
        code <<= 1;
        code |= previousBit;
      }
      previousBit = currentBit;
    }
  
  // Send the code via serial
  //if (i == DiOremote_FRAME_LENGTH) {
    Serial.println("");
    Serial.println("code " + String(code));
    if (code > 1404150000 && code < 1404160000) {
      Serial.println("OKOKOKOKOKOKOKOKOKOKOKOK");
    }
  //}
  }

}
