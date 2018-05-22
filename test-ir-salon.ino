/*
 * @corbanmailloux
 * https://github.com/corbanmailloux/ArduinoKodiIRRemote
 *
 * IR Remote ESP8266
 * https://github.com/markszabo/IRremoteESP8266/blob/master/examples/IRrecvDemo/IRrecvDemo.ino
 */

#include "constantes.h"

// WIFI + MQTT

#include <ESP8266WiFi.h>

const char* wifi_ssid  = WIFI_SSID;
const char* wifi_pwd   = WIFI_PASSWD;

WifiClient wifi_client;

// MQTT

#include <PubSubClient.h>

const char* mqtt_server = MQTT_SERVER;
const int   mqtt_port   = MQTT_PORT;
const char* mqtt_user   = MQTT_USER;
const char* mqtt_pwd    = MQTT_PWD;

PubSubClient mqtt(wifi_client);

// IR

#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

const int receivePin = 9;
IRrecv irrecv(receivePin);
decode_results results;

const int minimumPressTime = 100; // ms to keep the key pressed for

bool repeatable = false;
unsigned long pressTime = 0;
bool pressingKeys = false;
char* current_channel = NULL;
char* current_message = NULL;

void setup() {
  Serial.begin(74880);
  while (!Serial);

  Serial.println("Start");

  // TODO : wifi

  // TODO : mqtt

  // Start the IR receiver
  Serial.println("Start the IR receiver...");
  irrecv.enableIRIn();

  Serial.println("Done");
}

void loop() {
  if (irrecv.decode(&results)) {

    Serial.print("ir code : ");
    Serial.println(results.value, HEX);

    switch (results.value) {
      case 0xFFFFFFFF: // Repeat code
        // Don't release the key unless it's a non repeatable key.
        if (repeatable) {
          pressTime = millis(); // Keep the timer running.
        }
        break;

      // Leds

      case 0x2F0: // On - white
        repeatable = false;
          setAction("salon/leds", "on 200 200 200");
        break;
      case 0x2F0: // On - red
        repeatable = false;
          setAction("salon/leds", "on 200 10 10");
        break;
      case 0x2F0: // On - green
        repeatable = false;
          setAction("salon/leds", "on 10 200 10");
        break;
      case 0x2F0: // On - blue
        repeatable = false;
          setAction("salon/leds", "on 10 10 200");
        break;
      case 0x2F0: // Off
        repeatable = false;
          setAction("salon/leds", "off");
        break;

      // Video projecteur

      case 0x2F0: // On
        repeatable = false;
          setAction("salon/projecteur", "on");
        break;
      case 0x2F0: // Off
        repeatable = false;
        setAction("salon/projecteur", "off");
        break;

      // Kodi
      // Kodi addon : https://github.com/owagner/kodi2mqtt
      // Kodi python RPC : http://kodi.wiki/view/JSON-RPC_API#Python

      case 0x2F0: // Menu
        repeatable = true;
        setAction("salon/kodi", "menu");
        break;
      case 0x2F0: // Up
        repeatable = true;
        setAction("salon/kodi", "up");
        break;
      case 0xAF0: // Down
        repeatable = true;
        setAction("salon/kodi", "down");
        break;
      case 0x2D0: // Left
        repeatable = true;
        setAction("salon/kodi", "left");
        break;
      case 0xCD0: // Right
        repeatable = true;
        setAction("salon/kodi", "right");
        break;
      case 0xD10: // Enter
        repeatable = false;
        setAction("salon/kodi", "return");
        break;
      case 0x9B0: // Play/Pause
        repeatable = false;
        setAction("salon/kodi", "play_pause");
        break;
      case 0x9B0: // Stop
        repeatable = false;
        setAction("salon/kodi", "stop");
        break;
      case 0x1B0: // Rewind
        repeatable = false;
        setAction("salon/kodi", "rewind");
        break;
      case 0x7B0: // Fast Forward
        repeatable = false;
        setAction("salon/kodi", "forward");
        break;
      case 0x7B0: // Volume up
        repeatable = false;
        setAction("salon/kodi", "volume_up");
        break;
      case 0x7B0: // Volume down
        repeatable = false;
        setAction("salon/kodi", "volume_down");
        break;

      // Non-matching IR
      default:
        repeatable = false;
        break;
    }

    delay(25);
    irrecv.resume(); // Receive the next value
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

void setAction(char* channel, char* message) {
  pressingKeys = true;
  pressTime = millis();
  current_channel = channel;
  current_message = message;
  client.publish(channel, message);
}
