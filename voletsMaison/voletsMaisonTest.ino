/*
#include <ESP8266WiFi.h>
const char* wifi_ssid = "*************";
const char* wifi_pass = "*************";

void setup() {
  Serial.begin(9600);
  while (!Serial);

  WiFi.begin(wifi_ssid, wifi_pass);
}

byte relAON[] = {0xA0, 0x01, 0x01, 0xA2};
byte relAOFF[] = {0xA0, 0x01, 0x00, 0xA1};

byte relBON[] = {0xA0, 0x02, 0x01, 0xA3};
byte relBOFF[] = {0xA0, 0x02, 0x00, 0xA2};

void loop() {
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }

  Serial.write(relAON, sizeof(relAON));
  delay(1000);
  Serial.write(relAOFF, sizeof(relAOFF));
  delay(1000);
  Serial.write(relBON, sizeof(relBON));
  delay(1000);
  Serial.write(relBOFF, sizeof(relBOFF));
  delay(1000);
}
*/
