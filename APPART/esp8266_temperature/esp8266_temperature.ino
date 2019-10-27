
// --- WIFI ---------------------------------------------

#include <ESP8266WiFi.h>
#include <constants.h>
const char* wifi_ssid = WIFI_SSID;
const char* wifi_pass = WIFI_PASSWD;
IPAddress wifi_ipaddr(10, 0, 0, 102);
IPAddress wifi_gateway(10, 0, 0, 1);
IPAddress wifi_dns(10, 0, 0, 1);
IPAddress wifi_subnet(255, 255, 255, 0);
/**/

// --- MQTT ---------------------------------------------

#include <PubSubClient.h>
const char* mqtt_host = MQTT_SERVER;
const int   mqtt_port = MQTT_PORT;
const char* mqtt_user = MQTT_USER;
const char* mqtt_pwd  = MQTT_PWD;
const char* mqtt_client_id = "ArduinoTempChambre";
const char* mqtt_topic_data_temperature = "sensor/temperature/chambre";
const char* mqtt_topic_data_humidity = "sensor/humidity/chambre";
const char* mqtt_topic_data_battery = "sensor/battery/chambre";
WiFiClient wifiClient;
PubSubClient client(wifiClient);

// --- DHT22 ---------------------------------------------

#include "DHT.h"
#define DHTPIN D4
DHT dht(DHTPIN, DHT22);
float t, h;

// --- Battery check -------------------------------------

extern "C" {
  #include "user_interface.h"
}
ADC_MODE(ADC_VCC);
int vddtot = 0;
int vddcount = 0;

// --- delays --------------------------------------------

const int DELAY = 30*60; // 30 min

const int WIFI_ATTEMPTS = 50;
int wifi_errors = 0;

const int DHT_ATTEMPTS = 3;
int dht_errors = 0;

const int DELAY_ATTEMPTS = 100;

// ------------------------------------------------------

void setup() {
  /*
  Serial.begin(74880);
  while (!Serial);
  Serial.println("Start");
  */

  dht.begin();
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // led off
}


unsigned long mtime = millis();
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
      step_mqtt_init();
      break;
    case 4:
      step_dht_read();
      break;
    case 5:
      step_vdd33();
      break;
    case 6:
      send_data();
      break;
    case 7:
      //Serial.print(millis() - mtime);
      //Serial.println(" Sleeping...");
      deep_sleep(false);
      break;
  }
}


void step_wifi_init() {
  //Serial.print(millis() - mtime);
  //Serial.print(" Connecting to wifi...");
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
    //Serial.println();
    //Serial.print(millis() - mtime);
    //Serial.println(" OK");
    step++;
  }
  else {
    //Serial.print('.');
    wifi_errors++;
    delay(DELAY_ATTEMPTS);
     
    // give up this time
    if (wifi_errors > WIFI_ATTEMPTS) {
      //Serial.println();
      //Serial.print(millis() - mtime);
      //Serial.println(" FAIL ("+ String(wifi_errors) +"). Go to sleep");
      deep_sleep(true);
    }
  }
}

void step_mqtt_init() {
  client.setServer(mqtt_host, mqtt_port);
  if (!client.connected()) {
    client.connect(mqtt_client_id, mqtt_user, mqtt_pwd);
  }
  wifi_errors = 0;
  step++;
}

void step_mqtt_check() {
  if (client.connected()) {
    //Serial.println();
    //Serial.print(millis() - mtime);
    //Serial.println(" OK");
    step++;
  }
  else {
    //Serial.print('.');
    wifi_errors++;
    delay(DELAY_ATTEMPTS);
     
    // give up this time
    if (wifi_errors > WIFI_ATTEMPTS) {
      //Serial.println();
      //Serial.print(millis() - mtime);
      //Serial.println(" FAIL ("+ String(wifi_errors) +"). Go to sleep");
      deep_sleep(true);
    }
  }
}

void step_dht_read() {
  t = dht.readTemperature();
  h = dht.readHumidity();
  if (isnan(h) || isnan(t)) {
    dht_errors++;
    delay(DELAY_ATTEMPTS);
    if (dht_errors > DHT_ATTEMPTS) {
      //Serial.println();
      //Serial.print(millis() - mtime);
      //Serial.println(" FAIL ("+ String(wifi_errors) +"). Go to sleep");
      deep_sleep(true);
    }
  }
  else {
    step++;
  }
}

void step_vdd33() {
  vddtot += system_get_vdd33();
  vddcount++;
  step++;
}

void send_data() {
  client.publish(mqtt_topic_data_temperature, String(t).c_str(), true);
  client.publish(mqtt_topic_data_humidity, String(h).c_str(), true);
  client.publish(mqtt_topic_data_battery, String(vddtot / vddcount).c_str(), true);
  step++;
}

void deep_sleep(bool error) {
  float coeff;
  float v = vddtot / vddcount;
       if (error)    coeff = 1/3;              // 10 min
  else if (v > 3400) coeff = 1;                // 30 min
  else if (v < 3000) coeff = 6;                //  3 h
  else               coeff = 1+5*(3400-v)/400; // 30 min -> 3 h

  //Serial.println("Sleep for " + String(DELAY * coeff));
  //Serial.flush();
  ESP.deepSleep(DELAY * coeff * 1000000, WAKE_RF_DEFAULT);

  // reset for debug
  //delay(10000);
  //step = 0;
  //mtime = millis();

}
