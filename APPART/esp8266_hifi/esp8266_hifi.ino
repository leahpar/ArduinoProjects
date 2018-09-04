#include <constants.h>

// --- WIFI ---------------------------------------------

#include <ESP8266WiFi.h>
const char* wifi_ssid = WIFI_SSID;
const char* wifi_pass = WIFI_PASSWD;

// --- MQTT ---------------------------------------------

#include <PubSubClient.h>
const char* mqtt_host = MQTT_SERVER;
const int   mqtt_port = MQTT_PORT;
const char* mqtt_user = MQTT_USER;
const char* mqtt_pwd  = MQTT_PWD;
const char* mqtt_client_id = "ArduinoHifi";
const char* mqtt_topic = "salon/hifi";
WiFiClient wifiClient;
PubSubClient client(wifiClient);

// --- DIVERS ---------------------------------------------

const int PIN_RELAY = D2;
const int PIN_LED   = D3;
// Note: D4 = esp8266 led.

unsigned long t_current;
unsigned long t_previous;
unsigned long t_previous_wifi_check;
unsigned long t_previous_blink;
unsigned long t_hifi_toggle;
bool hifi_on = false;

void setup() {

  Serial.begin(74880);
  while (!Serial);
  Serial.println("Start");

  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, LOW); // Default : OFF

  pinMode(PIN_RELAY, OUTPUT);     // to relay
  digitalWrite(PIN_RELAY, LOW);   // Default : OFF

  pinMode(PIN_LED, OUTPUT);       // to led power
  digitalWrite(PIN_LED, LOW);     // Default : off


  // --- WIFI ---------------------------------------------

  WiFi.mode(WIFI_STA); // mode standard
  WiFi.begin(wifi_ssid, wifi_pass);
  reconnect_wifi();

  // --- MQTT ---------------------------------------------

  client.setServer(mqtt_host, mqtt_port);
  client.setCallback(mqtt_callback);
  reconnect_mqtt();

  Serial.println("Setup done");

  t_current  = millis();
  t_previous = millis();
  t_previous_wifi_check = millis();
  t_hifi_toggle = millis();

  blink(100,100);
  blink(100,100);
  blink(100,100);

}

void reconnect_wifi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to Wifi...");
    WiFi.begin(wifi_ssid, wifi_pass);
    int i = 0;
    while (WiFi.status() != WL_CONNECTED) {
      blink(20, 50 + 5*i++);
      Serial.print(".");
    }
    digitalWrite(BUILTIN_LED, HIGH);
    Serial.println();
  }
}

void reconnect_mqtt(){
  if (!client.connected()) {
    Serial.print("Connecting to MQTT...");

    int i = 0;
    while (!client.connect(mqtt_client_id, mqtt_user, mqtt_pwd)) {
      blink(20, 50 + 5*i++);
      Serial.print(".");
    }
    digitalWrite(BUILTIN_LED, HIGH);
    Serial.println();

    Serial.println("Subscribe to channels...");
    client.subscribe(mqtt_topic);

    client.publish("hello/world", mqtt_client_id);
  }
}


void loop() {

  t_current = millis();

  if ((unsigned long)(t_current - t_previous_wifi_check) >= 10000) {
    // Every 10 seconds, check wifi & mqtt connections
    t_previous_wifi_check = t_current;
    reconnect_wifi();
    reconnect_mqtt();
  }

  // Check subscribed mqtt channels
  client.loop();

  delay(10);
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String action;

  payload[length] = '\0';
  String message = String((char*)payload);

  action = message;

  Serial.println("[" + String(topic) + "] " + message);
  blink(30, 0);

  if (action == "off") {
    Serial.println("Hifi OFF");
    digitalWrite(PIN_RELAY, LOW);
    delay(10);
    digitalWrite(PIN_LED, LOW);
  }

  else if (action == "on") {
    Serial.println("Hifi ON");
    digitalWrite(PIN_RELAY, HIGH);
    delay(10);
    digitalWrite(PIN_LED, HIGH);
  }

  else if (action == "toggle") {
    if ((unsigned long)(millis() - t_hifi_toggle) > 2000) {
      t_hifi_toggle = millis();
      if (hifi_on) {
        hifi_on = false;
        Serial.println("Hifi toggle off");
        digitalWrite(PIN_RELAY, LOW);
        delay(10);
        digitalWrite(PIN_LED, LOW);
      }
      else {
        hifi_on = true;
        Serial.println("Hifi toggle on");
        digitalWrite(PIN_RELAY, HIGH);
        delay(10);
        digitalWrite(PIN_LED, HIGH);
      }
    }
  }
}

 void blink(int up, int down) {
  digitalWrite(BUILTIN_LED, LOW);
  delay(up);
  digitalWrite(BUILTIN_LED, HIGH);
  delay(down);
}
