/*
 * http://10.0.0.47/?echo=15&temperature=1&humidity=1&wifi=1
 * 
 */

// --- WIFI ------------------------------------------------------------------

#include <ESP8266WiFi.h>
const char* wifi_ssid = "TATOOINE";
const char* wifi_pass = "yapademotdepasse";

// --- Serveur HTTP ----------------------------------------------------------

#include <ESP8266WebServer.h>
ESP8266WebServer server(80);

// --- Capteur distance HC-SR04 ----------------------------------------------

const byte TRIGGER_PIN = D7;
const byte ECHO_PIN    = D6;
const unsigned long MEASURE_TIMEOUT = 25000UL; // 25ms = ~8m

// --- Capteur température DHT22 ---------------------------------------------

#include <DHT.h>
#define DHTTYPE DHT22
DHT dht(D5, DHTTYPE);
float humidity;
float temperature;
unsigned long chrono;

// --- Mosfet -----------------------------------------------------------------

const byte POWER_PIN = D2;


//==============================================================================
// SETUP
//==============================================================================
void setup() {

  // Serial
  
  Serial.begin(74880);
  while (!Serial);
  
  Serial.setDebugOutput(true);
  Serial.println("");
  Serial.println("");
  Serial.println(ESP.getResetReason());

  // Wifi
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_pass);

  // LED
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // Distance
  
  pinMode(TRIGGER_PIN, INPUT); // INPUT pour couper le courant
  digitalWrite(TRIGGER_PIN, LOW);
  pinMode(ECHO_PIN, INPUT);

  // Temperature
  
  dht.begin();

  // Power
  
  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, LOW);
  
  // HTTP server
  
  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");
}

//==============================================================================
// LOOP
//==============================================================================
void loop(void){
  server.handleClient();
  delay(10);
}

//==============================================================================
// HTTP server : /
//==============================================================================
void handleRoot() {
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(POWER_PIN, HIGH);
  
  String response = "";

  if (server.hasArg("echo")) {
    pinMode(TRIGGER_PIN, OUTPUT); // OUTPUT pour envoyer le signal
    delay(10);
    response += "echo=" + String(measure_distance());
    int echos = server.arg("echo").toInt()-1;
    for (int i=0; i<echos; i++) {
      digitalWrite(LED_BUILTIN, HIGH);
      response += ";" + String(measure_distance());
      delay(50);
      digitalWrite(LED_BUILTIN, LOW);
      delay(50);
    }
    response += "\n";
    pinMode(TRIGGER_PIN, INPUT); // INPUT pour couper le courant
  
  }

  if (server.hasArg("temperature") || server.hasArg("humidity")) {
    measure_temperature();
  }
  
  if (server.hasArg("temperature")) {
    response += "temperature=" + String(temperature) + "\n";
  }

  if (server.hasArg("humidity")) {
    response += "humidity=" + String(humidity) + "\n";
  }

  if (server.hasArg("wifi")) {
    response += "wifi=" + String(WiFi.RSSI()) + "\n";
  }

  server.send(200, "text/plain", response);
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(POWER_PIN, LOW);
}


//==============================================================================
// Measure : distance
//==============================================================================
long measure_distance() {
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  long measure = pulseIn(ECHO_PIN, HIGH, MEASURE_TIMEOUT);
  return measure;
}

//==============================================================================
// Measure : temperature & humidity
//==============================================================================
void measure_temperature() {
  chrono = millis(); // for timeout

  do {
    delay(500);
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();
    Serial.println(String(humidity) + " " + String(temperature));
  } while (isnan(humidity) && isnan(temperature) && millis() - chrono < 7000);
}



