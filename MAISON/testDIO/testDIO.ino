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

#include <DiOremote.h>


// --- HomeEasy protocol parameters ---
#define DiOremote_PULSEIN_TIMEOUT 1000000
#define DiOremote_FRAME_LENGTH 64
// Homeasy start lock : 2725 us (+/- 175 us)
#define DiOremote_START_TLOW 2550
#define DiOremote_START_THIGH 2900
// Homeeasy 0 : 310 us
#define DiOremote_0_TLOW 250
#define DiOremote_0_THIGH 450
//Homeeasy 1 : 1300 us
#define DiOremote_1_TLOW 1250
#define DiOremote_1_THIGH 1500

// --- Variables ---

const int RX_PIN = D2;
const int TX_PIN = D1;


unsigned long codeRx = 0;         // Radio  -> Serial
unsigned long codeTx = 0;         // Serial -> Radio
int codeLen;
unsigned long t = 0;              // Time between two falling edges
byte currentBit,previousBit;
String codeTxStr;
const unsigned long TEST_CODE  = 1278825104;

DiOremote myRemote = DiOremote(TX_PIN);


void setup() {
  pinMode(RX_PIN, INPUT);
  pinMode(TX_PIN, OUTPUT);

  Serial.begin(74880);
  while (!Serial);
  Serial.println(" Version: "+ String(_DATE_) + " - " + String(_TIME_) + "\n");
  
  Serial.println("Test : " + String(TEST_CODE));
  myRemote.send(TEST_CODE);
  Serial.println("Done");

}

void loop() {

  readRadio();

  // If complete code, print it
  if (codeLen == DiOremote_FRAME_LENGTH) {
    Serial.println("Received: " + String(codeRx));
  }

  // Read Serial
  if (Serial.available()) {
    Serial.println("...");
    codeTxStr = Serial.readStringUntil('\n');
    codeTx = codeTxStr.toInt();
    myRemote.send(codeTx);
    Serial.println("Send: " + String(codeTx));
  }
}



void readRadio() {
  // Wait for incoming bit
  t = pulseIn(RX_PIN, LOW, DiOremote_PULSEIN_TIMEOUT);

  // Only decypher from 2nd Homeeasy lock
  if (t > DiOremote_START_TLOW && t < DiOremote_START_THIGH) {

    for (codeLen = 0 ; codeLen < DiOremote_FRAME_LENGTH ; codeLen++) {
      // Read each bit (64 times)
      t = pulseIn(RX_PIN, LOW, DiOremote_PULSEIN_TIMEOUT);

      // Identify current bit based on the pulse length
      if (t > DiOremote_0_TLOW && t < DiOremote_0_THIGH)
        currentBit = 0;
      else if (t > DiOremote_1_TLOW && t < DiOremote_1_THIGH)
        currentBit = 1;
      else
        break;

      // If bit count is even, check Manchester validity & store in code
      if (codeLen % 2) {

        // Code validity verification (Manchester complience)
        if (!(previousBit ^ currentBit))
          break;

        // Store new bit
        codeRx <<= 1;
        codeRx |= previousBit;
      }
      previousBit = currentBit;
    }
  }
}
