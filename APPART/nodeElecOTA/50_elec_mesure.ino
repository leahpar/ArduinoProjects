#include <ESP8266WiFi.h>
#include "RemoteDebug.h"

const int PIN_INTERRUPT = D5;
volatile unsigned long elec_curpulse = 0;
volatile unsigned long elec_lastpulse = 0;
volatile int elec_P = 0;              // puissance instantanée en W
//volatile int Pmax = 0;           // pic de puissance en W
unsigned long elec_cptEDF = 0;        // total conso en Wh

long elec_chrono;
const int elec_measureDelay = 10 * 60 * 1000; // 10 min


void elec_setup() {
    pinMode(PIN_INTERRUPT, INPUT_PULLUP);
    attachInterrupt(PIN_INTERRUPT, elec_pulseInterrupt, FALLING);
    elec_chrono = millis();
    elec_curpulse = millis();
    elec_lastpulse = 0;
}

void elec_handle() {
    if (millis() - elec_chrono > elec_measureDelay) {
    // Send data
    rdebugIln("Send data [%s] %s", mqtt_topic_data, String(elec_cptEDF).c_str());
    mqtt_connect();
    client.publish(mqtt_topic_data, String(elec_cptEDF).c_str(), true);
    elec_cptEDF = 0;
    elec_chrono = millis();
  }
}


//==============================================================================
// PULSE INTERRUPT
//==============================================================================
void elec_pulseInterrupt() {
  elec_curpulse = millis();
  if (elec_curpulse - elec_lastpulse > 400) { // max 9kWh
    elec_P = 3600000 / (elec_curpulse - elec_lastpulse);
    //if (P > Pmax) Pmax = P;
    elec_cptEDF++;
    //Serial.println("Pulse");
    elec_lastpulse = elec_curpulse;
  }
}
