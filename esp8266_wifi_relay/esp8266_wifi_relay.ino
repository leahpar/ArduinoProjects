/*

https://www.hackster.io/makerrelay/esp8266-wifi-5v-1-channel-relay-delay-module-iot-smart-home-e8a437
http://blog.nicolasc.eu/esp8266-seconde-partie-le-mode-standalone/
https://nodemcu-build.com/index.php
https://techtutorialsx.com/2017/06/04/esp32-esp8266-micropython-uploading-files-to-the-file-system/

*/

#include <constants.h>

#include <ESP8266WiFi.h>
const char* wifi_ssid = WIFI_SSID;
const char* wifi_pass = WIFI_PASSWD;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  
  Serial.begin(9600);
  while (!Serial);
  Serial.println("Start");

  WiFi.begin(wifi_ssid, wifi_pass);
}

byte relON[] = {0xA0, 0x01, 0x01, 0xA2};
byte relOFF[] = {0xA0, 0x01, 0x00, 0xA1};
//byte relBON[] = {0xA0, 0x02, 0x01, 0xA3};
//byte relBOFF[] = {0xA0, 0x02, 0x00, 0xA2};


// the loop function runs over and over again forever
void loop() {

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }

  Serial.write(relON, sizeof(relON));
  //digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  Serial.write(relOFF, sizeof(relOFF));
  //digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
  Serial.println("loop");
}
