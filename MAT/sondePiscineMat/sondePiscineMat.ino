
// --- WIFI ---------------------------------------------

#include <ESP8266WiFi.h>

/* RAF 
#include <constants.h>
const char* wifi_ssid = WIFI_SSID;
const char* wifi_pass = WIFI_PASSWD;
IPAddress wifi_ipaddr(10, 0, 0, 210);
IPAddress wifi_gateway(10, 0, 0, 1);
IPAddress wifi_dns(10, 0, 0, 1);
IPAddress wifi_subnet(255, 255, 255, 0);
/**/

/* MAT */
const char* wifi_ssid = "thewifi";
const char* wifi_pass = "*********";
IPAddress wifi_ipaddr(192, 168, 1, 99);
IPAddress wifi_gateway(192, 168, 1, 1);
IPAddress wifi_dns(192, 168, 1, 1);
IPAddress wifi_subnet(255, 255, 255, 0);
/**/

// --- HTTP ---------------------------------------------

#include <ESP8266HTTPClient.h>
const char* host = "raphael.bacco.fr";
const int   port = 80;
//WiFiClient client;
HTTPClient http;


// --- SONDE ---------------------------------------------

// https://create.arduino.cc/projecthub/TheGadgetBoy/ds18b20-digital-temperature-sensor-and-arduino-9cc806
// https://letmeknow.fr/blog/2016/08/10/le-capteur-de-temperature-ds18b20/

#include <OneWire.h> //Librairie du bus OneWire
#include <DallasTemperature.h> //Librairie du capteur

OneWire oneWire(D7); //Bus One Wire sur la pin 2 de l'arduino
DallasTemperature sensors(&oneWire); //Utilistion du bus Onewire pour les capteurs
DeviceAddress sensorDeviceAddress; //Vérifie la compatibilité des capteurs avec la librairie

float t = NULL;

// --- Battery check -------------------------------------

extern "C" {
  #include "user_interface.h"
}
ADC_MODE(ADC_VCC);


const int DELAY = 30*60; // 30 min
//const int DELAY = 10;

const int WIFI_ATTEMPTS = 50;
int wifi_errors = 0;

const int DELAY_ATTEMPTS = 100;


void setup() {
  Serial.begin(74880);
  while (!Serial);
  Serial.println("Start");
  
  //pinMode(BUILTIN_LED, OUTPUT);
  //digitalWrite(BUILTIN_LED, LOW);

  sensors.begin(); //Activation des capteurs
  sensors.getAddress(sensorDeviceAddress, 0); //Demande l'adresse du capteur à l'index 0 du bus
  sensors.setResolution(sensorDeviceAddress, 10); //Résolutions possibles: 9,10,11,12
}


unsigned long mtime = millis();
int vddtot = 0;
int vddcount = 0;
int step = 0;
int c = 0;
String str;
int errors = 0;


void loop() {
  // put your main code here, to run repeatedly:
  switch (step) {
    case 0:
      step_vdd33();
      break;
    case 1:
      step_wifi_init();
      break;
    case 2:
      step_wifi_check();
      break;
    case 3:
      step_dht_read();
      break;
    case 4:
      step_vdd33();
      break;
    case 5:
      send_data();
      break;
    case 6:
      Serial.print(millis() - mtime);
      Serial.println(" Sleeping...");
      ESP.deepSleep(DELAY * 1000000 , WAKE_RF_DEFAULT);
      // Dead code, comment line above to debug without deepsleep
      delay(DELAY * 1000);
      step = 0;
      mtime = millis();
      break;
  }
}


void step_wifi_init() {
  Serial.print(millis() - mtime);
  Serial.print(" Connecting to wifi...");
  // ? https://github.com/esp8266/Arduino/issues/2186
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.config(wifi_ipaddr, wifi_dns, wifi_gateway, wifi_subnet);
  WiFi.begin(wifi_ssid, wifi_pass);
  wifi_errors = 0;
  step++;
}

void step_wifi_check() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print(millis() - mtime);
    Serial.println(" OK");
    step++;
  }
  else {
    Serial.print('.');
    wifi_errors++;
    delay(DELAY_ATTEMPTS);
     
    // give up this time
    if (wifi_errors > WIFI_ATTEMPTS) {
      Serial.println();
      Serial.print(millis() - mtime);
      Serial.println(" FAIL ("+ String(wifi_errors) +"). Go to sleep");
      Serial.flush();
      ESP.deepSleep(DELAY / 3 * 1000000 , WAKE_RF_DEFAULT);
    }
  }
}

void step_dht_read() {
  sensors.requestTemperatures();
  t = sensors.getTempCByIndex(0);
  //Serial.print(sensors.getTempCByIndex(0));
  //Serial.println(" °C");
  
  step++;
}

void step_vdd33() {
  vddtot += system_get_vdd33();
  vddcount++;
  step++;
}

void send_data() {
  /*
  if (!client.connect(host, 80)) {
    Serial.println("connection failed");
    delay(50);
    return;
  }
  */
  
  String url = String("/piscine.php")
             + String("?t=") + String(t)
             + String("&v=") + String(vddtot / vddcount);
  
  Serial.print("Requesting URL: ");
  Serial.println(url);
  
  /*
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 2000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }
  */
  
  http.begin(host, port, url);

  int httpCode = http.GET();
  Serial.println(httpCode);
  
  Serial.println("closing connection");
  http.end();
  
  Serial.println("done");
  step++;
}

