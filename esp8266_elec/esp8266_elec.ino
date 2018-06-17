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

const int PIN_INTERRUPT = D6;
volatile unsigned long curpulse = 0;
volatile unsigned long lastpulse = 0;
volatile int P = 0;           // puissance instantanée en W
volatile int Pmax = 0;           // pic de puissance en W
unsigned long cptEDF = 0;     // total conso heures pleines en Wh

// --- OLED ------------------------------------------------------------------

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET LED_BUILTIN
Adafruit_SSD1306 display(OLED_RESET);

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

// --- Misc ------------------------------------------------------------------

long chrono;
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
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  Serial.println("?");
  display.display();
  delay(1000);

  // --- Pulse ------------------------------------------------------------------

  pinMode(PIN_INTERRUPT, INPUT_PULLUP);
  attachInterrupt(PIN_INTERRUPT, pulseInterrupt, FALLING);
  chrono = millis();
  curpulse = millis();
  lastpulse = 0;

  Serial.println("Setup done");
}


//==============================================================================
// PULSE INTERRUPT
//==============================================================================
void pulseInterrupt() {
  curpulse = millis();
  if (curpulse - lastpulse > 400) {
    P = 3600000 / (curpulse - lastpulse);
    if (P > Pmax) Pmax = P;
    cptEDF++;
    Serial.println("Pulse");
    lastpulse = curpulse;
  }
}

//==============================================================================
// LOOP
//==============================================================================
void loop() {
  if (millis() - chrono > measureDelay) {
    // Send data
    connectWifi();
    connectMqtt();
    //client.publish(mqtt_topic_data, String(cptEDF).c_str());
    chrono = millis();
  }

  oled_display();
  Serial.print(".");
  delay(500);
}

//==============================================================================
// OLED display
//==============================================================================
void oled_display() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(strpad(String(P), 6) + " W");
  display.println(strpad(String(Pmax), 6) + " Wm");
  display.println(strpad(String(cptEDF/1000.0), 6) + " kWh");
  display.display();
}
String strpad(String str, int l) {
  while (str.length() < l) str = String(" ") + str;
  return str;
}

//==============================================================================
// CONNECT WIFI
//==============================================================================
bool connectWifi() {
  Serial.println("connectWifi()");
  WiFi.begin(wifi_ssid, wifi_pass);
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    blink(20, 50 + 5*i++);
  }
  return true;
}

//==============================================================================
// CONNECT MQTT
//==============================================================================
bool connectMqtt() {
  Serial.println("connectMqtt()");
  int i = 0;
  while (!client.connect(mqtt_client_id, mqtt_user, mqtt_pwd)) {
    blink(20, 50 + 5*i++);
  }
  return true;
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

