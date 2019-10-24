/**
 * FauxmoESP
 * https://bitbucket.org/xoseperez/fauxmoesp
 * 
 * 
 * Voir aussi : ESPAlexa
 * https://github.com/Aircoookie/Espalexa
 * (non utilisé ici, mais piste pour plus tard)
 */

#include <Arduino.h>
#include <constants.h>

// --- WIFI ---------------------------------------------

#include <ESP8266WiFi.h>
const char* wifi_ssid = WIFI_SSID;
const char* wifi_pass = WIFI_PASSWD;

// --- Alexa --------------------------------------------

#include "fauxmoESP.h"
fauxmoESP fauxmo;
const char* alexa_device_leds = "leds du bureau";
const char* alexa_device_yoda = "yoda";
//const char* alexa_device_03 = "K2000";

// --- Wake on lan -------------------------------------

#include <WiFiUDP.h>
#include <WakeOnLan.h>
#include <ESP8266Ping.h>
WiFiUDP UDP;
const IPAddress wol_ip(10,0,0,21);
const IPAddress wol_broadcast(10,0,0,255);
// yoda 10.0.0.21 4C:CC:6A:4C:48:6A
byte wol_mac[] = { 0x4C, 0xCC, 0x6A, 0x4C, 0x48, 0x6A };
int wol_cpt = 0;
bool wol_up = false;

// --- Sleep on lan ------------------------------------

WiFiClient client;
const int telnetPort = 29800;
const byte yoda_ip[] = { 10, 0, 0, 21 };
bool wol_down = false;

// --- Leds --------------------------------------------

#include <Adafruit_NeoPixel.h>
#define PIN_LEDS D4
#define NUMPIXELS 101
#define BRIGHTNESS 128
Adafruit_NeoPixel pixels(NUMPIXELS, PIN_LEDS, NEO_RGB + NEO_KHZ800);
uint32_t black = pixels.Color(0, 0, 0);

bool leds_state = false;
unsigned char leds_value = 0;

// DELL
const int leds_dell_start = 0;
const int leds_dell_end = 18;
  
// ASUS
const int leds_asus_start = 40;
const int leds_asus_end = 79;
  
// ACER
const int leds_acer_start = 90;
const int leds_acer_end = 100;

// K2000
bool k2000 = false;


// -----------------------------------------------------------------------------
// SETUP
// -----------------------------------------------------------------------------

void setup() {

  // Builtin led => ON
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Serial
  Serial.begin(74880);
  while (!Serial);
  Serial.println("[SETUP] Start");

  // Pixels
  Serial.println("[SETUP] Pixels");
  pixels.begin();
  pixels.clear();
  pixels.setBrightness(BRIGHTNESS);
  pixels.show();

  // Wifi
  Serial.println("[SETUP] Wifi");
  wifiSetup();

  // Fauxmo
  Serial.println("[SETUP] FauxMO");
  fauxmoSetup();

  // WOL
  UDP.begin(9); //start UDP client, not sure if really necessary.

  Serial.println("[SETUP] Done");
  delay(500);

  // Builtin led => OFF
  digitalWrite(LED_BUILTIN, HIGH);
} 


// -----------------------------------------------------------------------------
// LOOP
// -----------------------------------------------------------------------------

void loop() {
  fauxmo.handle();

  // Update leds every X ms
  static unsigned long last_led = millis();
  if (millis() - last_led > 20) {
    last_led = millis();
    updateLeds();
  }

  // Send WOL packets
  static unsigned long last_wol = millis();
  if (millis() - last_wol > 1000 && wol_cpt > 0) {
    last_wol = millis();
    wol_cpt--;
    WakeOnLan::sendWOL(wol_broadcast, UDP, wol_mac, sizeof wol_mac);
  }

  // Shut down Ypda
  if (wol_down) {
    wol_down = false;
    if (client.connect(yoda_ip, telnetPort)) {
      client.print(String("stop\r\n"));
      client.stop();
    }
    else {
       Serial.println("connection failed");      
    } 
  }
  
  // Update yoda status
  static unsigned long last_ping = millis();
  if (millis() - last_ping > 15000) {
    last_ping = millis();
    wol_up = Ping.ping(wol_ip, 2);
    Serial.println("[PING] Update " + String(wol_up));
    fauxmo.setState(alexa_device_yoda, wol_up, 255);
  }
  

  // This is a sample code to output free heap every 5 seconds
  // This is a cheap way to detect memory leaks
  static unsigned long last = millis();
  if (millis() - last > 15000) {
      last = millis();
      Serial.printf("[MAIN] Free heap: %d bytes\n", ESP.getFreeHeap());
  }

  // If your device state is changed by any other means (MQTT, physical button,...)
  // you can instruct the library to report the new state to Alexa on next request:
  // fauxmo.setState(ID_YELLOW, true, 255);

  delay(5);
}


// -----------------------------------------------------------------------------
// Wifi
// -----------------------------------------------------------------------------

void wifiSetup() {
    // Set WIFI module to STA mode
    WiFi.mode(WIFI_STA);

    // Connect
    Serial.printf("[WIFI] Connecting to %s ", wifi_ssid);
    WiFi.begin(wifi_ssid, wifi_pass);

    // Wait
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
    }
    Serial.println();

    // Connected!
    Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
}

// -----------------------------------------------------------------------------
// FauxMO
// -----------------------------------------------------------------------------

void fauxmoSetup() {
    // By default, fauxmoESP creates it's own webserver on the defined port
    // The TCP port must be 80 for gen3 devices (default is 1901)
    // This has to be done before the call to enable()
    fauxmo.createServer(true); // not needed, this is the default value
    fauxmo.setPort(80); // This is required for gen3 devices

    // You have to call enable(true) once you have a WiFi connection
    // You can enable or disable the library at any moment
    // Disabling it will prevent the devices from being discovered and switched
    fauxmo.enable(true);

    // You can use different ways to invoke alexa to modify the devices state:
    // "Alexa, turn yellow lamp on"
    // "Alexa, turn on yellow lamp
    // "Alexa, set yellow lamp to fifty" (50 means 50% of brightness, note, this example does not use this functionality)

    // Add virtual devices
    fauxmo.addDevice(alexa_device_leds);
    fauxmo.addDevice(alexa_device_yoda);
    //fauxmo.addDevice(alexa_device_03);

    fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {
        
        // Callback when a command from Alexa is received. 
        // You can use device_id or device_name to choose the element to perform an action onto (relay, LED,...)
        // State is a boolean (ON/OFF) and value a number from 0 to 255 (if you say "set kitchen light to 50%" you will receive a 128 here).
        // Just remember not to delay too much here, this is a callback, exit as soon as possible.
        // If you have to do something more involved here set a flag and process it in your main loop.

        digitalWrite(LED_BUILTIN, LOW);
        Serial.printf("[ALEXA] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);

        // Checking for device_id is simpler if you are certain about the order they are loaded and it does not change.
        // Otherwise comparing the device_name is safer.

        if (strcmp(device_name, alexa_device_leds) == 0) {
          if (!state) {
            leds_value = 0;
          }
          else {
            leds_value = map(value, 0, 255, 0, BRIGHTNESS);
            //leds_value = value;
          }
        }
        else if (strcmp(device_name, alexa_device_yoda) == 0) {
          if (state == true) {
            wol_cpt = 5;
          }
          else {
            wol_down = true;
          }
        }
        /*
        else if (strcmp(device_name, alexa_device_03) == 0) {
          k2000 = state;
        }
        */

        digitalWrite(LED_BUILTIN, HIGH);
    });
}

// -----------------------------------------------------------------------------
// Leds
// -----------------------------------------------------------------------------

void updateLeds() {
  //digitalWrite(LED_YELLOW, state ? HIGH : LOW);

  static unsigned char leds_value_previous = 0;
  if (leds_value != leds_value_previous) {
    
    //Serial.println("[LEDS] Update " + String(leds_value_previous));
    leds_value_previous = leds_value_previous + (leds_value-leds_value_previous) / abs(leds_value-leds_value_previous);

    pixels.clear();

    int c = map(leds_value_previous, 0, BRIGHTNESS, 0, BRIGHTNESS/3);

    uint32_t color = pixels.Color(10, 0, leds_value_previous);

    // DELL
    for (int i=leds_dell_start; i<leds_dell_end; i++) {
      pixels.setPixelColor(i, color);
    }
    //pixels.fill(color, leds_dell_start, leds_dell_end-leds_dell_start);
    
    // ASUS
    for (int i=leds_asus_start; i<leds_asus_end; i++) {
      pixels.setPixelColor(i, color);
    }
    //pixels.fill(color, leds_asus_start, leds_asus_end-leds_asus_start);
    
    // ACER
    for (int i=leds_acer_start; i<leds_acer_end; i++) {
      pixels.setPixelColor(i, color);
    }
    //pixels.fill(color, leds_acer_start, leds_acer_end-leds_acer_start);
    
    pixels.show();
    delay(1);
  }

  else if (k2000) {
      static int k2_pos = leds_asus_start;
      static int k2_dir = 1;

      pixels.clear();
      for (int i=0; i<5; i++) {
        uint32_t color = pixels.Color(255/(i+1), 0, 0);
        pixels.setPixelColor(k2_pos+i, color);
        pixels.setPixelColor(k2_pos-i, color);
      }
      for (int i=0; i<5; i++) {
        pixels.setPixelColor(leds_asus_start-i, black);
        pixels.setPixelColor(leds_asus_end+i, black);
      }
      pixels.show();

      k2_pos += k2_dir;
      if (k2_pos == leds_asus_end) {
        k2_dir = -1;
      }
      else if (k2_pos == leds_asus_start) {
        k2_dir = 1;
      }
      
      delay(20);

  }

}
