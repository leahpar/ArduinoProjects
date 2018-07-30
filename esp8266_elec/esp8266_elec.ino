/*
 * Watchdog doc
 * https://tushev.org/articles/arduino/5/arduino-and-watchdog-timer
 *
 * Other doc (with reset values explainations)
 * https://www.sigmdel.ca/michel/program/esp8266/arduino/watchdogs_en.html
 */

#include <constants.h>

// --- WIFI --------------------------------------------------------------------

#include <ESP8266WiFi.h>
const char* wifi_ssid = WIFI_SSID;
const char* wifi_pass = WIFI_PASSWD;

// --- HTTP Server HTTP --------------------------------------------------------

//#include <ESP8266WebServer.h>
//ESP8266WebServer server(80);

// --- MQTT --------------------------------------------------------------------

#include <PubSubClient.h>
const char* mqtt_host = MQTT_SERVER;
const int   mqtt_port = MQTT_PORT;
const char* mqtt_user = MQTT_USER;
const char* mqtt_pwd  = MQTT_PWD;
const char* mqtt_client_id = "ArduinoElec";
const char* mqtt_topic_data = "sensor/elec/appart";
WiFiClient wifiClient;
PubSubClient client(wifiClient);

// --- Pulse -----------------------------------------------------------------

const int PIN_INTERRUPT = D5;
volatile unsigned long curpulse = 0;
volatile unsigned long lastpulse = 0;
volatile int P = 0;              // puissance instantanée en W
//volatile int Pmax = 0;           // pic de puissance en W
unsigned long cptEDF = 0;        // total conso en Wh

// --- OLED ------------------------------------------------------------------

#define WITH_OLED false

// SCL <=> D1
// SDA <=> D2
// VCC <=> 3.3v
// GND <=> GND

#if (WITH_OLED)
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET LED_BUILTIN
Adafruit_SSD1306 display(OLED_RESET);

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif // SSD1306_LCDHEIGHT
#endif // WITH_OLED

// --- Misc ------------------------------------------------------------------

long chrono;
long chrono_alive;
const int measureDelay = 10 * 60 * 1000; // 10 min


//==============================================================================
// SETUP
//==============================================================================
void setup() {
  Serial.begin(74880);
  while (!Serial);
  Serial.println("Start");

  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH); // led off

  // --- WIFI ------------------------------------------------------------------

  WiFi.mode(WIFI_STA); // mode standard
  WiFi.begin(wifi_ssid, wifi_pass);
  //connectWifi();

  // --- MQTT ------------------------------------------------------------------

  client.setServer(mqtt_host, mqtt_port);
  //connectMqtt();

  // --- Oled -------------------------------------------------------------------
  #if (WITH_OLED)
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.display();
    delay(1000);
  #endif

  // --- Pulse ------------------------------------------------------------------

  pinMode(PIN_INTERRUPT, INPUT_PULLUP);
  attachInterrupt(PIN_INTERRUPT, pulseInterrupt, FALLING);
  chrono = millis();
  chrono_alive = millis();
  curpulse = millis();
  lastpulse = 0;

  Serial.println("Setup done");
  blink(20, 50);
  blink(20, 50);
  blink(20, 50);
  blink(20, 50);
  blink(20, 50);
}


//==============================================================================
// PULSE INTERRUPT
//==============================================================================
void pulseInterrupt() {
  curpulse = millis();
  if (curpulse - lastpulse > 400) { // max 9kWh
    P = 3600000 / (curpulse - lastpulse);
    //if (P > Pmax) Pmax = P;
    cptEDF++;
    //Serial.println("Pulse");
    lastpulse = curpulse;
  }
}

//==============================================================================
// LOOP
//==============================================================================
void loop() {

  // Check connection every 5 sec
  if (millis() - chrono_alive > 5000) {
    connectWifi();
    connectMqtt();
    blink(50,0);
    chrono_alive = millis();
  }
  
  if (millis() - chrono > measureDelay) {
    // Send data
    client.publish(mqtt_topic_data, String(cptEDF).c_str());
    cptEDF = 0;
    chrono = millis();
  }

  oled_display();
  //Serial.print(".");
  delay(500);
}

//==============================================================================
// OLED display
//==============================================================================
void oled_display() {
  #if (WITH_OLED)
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println(strpad(String(P), 6) + " W");
    //display.println(strpad(String(Pmax), 6) + " Wm");
    display.println(strpad(String(cptEDF/1000.0), 6) + " kWh");
    display.display();
  #endif
}
String strpad(String str, int l) {
  while (str.length() < l) str = String(" ") + str;
  return str;
}

//==============================================================================
// CONNECT WIFI
//==============================================================================
void connectWifi() {
  //Serial.println("connectWifi()");
  while (WiFi.status() != WL_CONNECTED) {
    // Just wait to reconnect to wifi
    blink(50, 500);
  }
}

//==============================================================================
// CONNECT MQTT
//==============================================================================
void connectMqtt() {
  //Serial.println("connectMqtt()");
  while (!client.connected()) {
    client.connect(mqtt_client_id, mqtt_user, mqtt_pwd);
    blink(50, 2000);
  }
}

//==============================================================================
// BLINK BUILTIN LED
//==============================================================================
void blink(int up, int down) {
  digitalWrite(BUILTIN_LED, LOW);
  delay(up);
  digitalWrite(BUILTIN_LED, HIGH);
  delay(down);
}
