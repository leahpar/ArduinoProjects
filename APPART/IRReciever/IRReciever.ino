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
const char* mqtt_client_id = "ArduinoIR";
WiFiClient wifiClient;
PubSubClient client(wifiClient);

// --- IR ---------------------------------------------

#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

const int receivePin = D2;
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

  pinMode(BUILTIN_LED, OUTPUT);

  // --- WIFI ---------------------------------------------

  WiFi.mode(WIFI_STA); // mode standard
  WiFi.begin(wifi_ssid, wifi_pass);
  reconnect_wifi();

  // --- MQTT ---------------------------------------------
  
  client.setServer(mqtt_host, mqtt_port);
  //client.setCallback(mqtt_callback);
  reconnect_mqtt();

  // --- IR ---------------------------------------------
  
  Serial.println("Start the IR receiver...");
  irrecv.enableIRIn();

  Serial.println("Done");
}

void blink(int up, int down) {
  digitalWrite(BUILTIN_LED, LOW);
  delay(up);
  digitalWrite(BUILTIN_LED, HIGH);
  delay(down);
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
    
    //Serial.println("Subscribe to channels...");
    //client.subscribe(mqtt_topic);

    client.publish("hello/world", mqtt_client_id);
  }
}

void loop() {
  if (irrecv.decode(&results)) {

    reconnect_wifi();
    reconnect_mqtt();

    Serial.print("ir code : ");
    serialPrintUint64(results.value, HEX);
    Serial.println("");
    
    switch (results.value) {
      case 0xFFFFFFFF: // Repeat code
        // Don't release the key unless it's a non repeatable key.
        if (repeatable) {
          pressTime = millis(); // Keep the timer running.
        }
        break;

      // Leds

      case 0x10ED00FF: // Color - white
        repeatable = false;
          setAction("salon/leds", "color 200 200 200");
        break;
      case 0x10ED42BD: // Color - red
        repeatable = false;
          setAction("salon/leds", "color 255 10 10");
        break;
      case 0x10ED02FD: // Color - green
        repeatable = false;
          setAction("salon/leds", "color 10 255 10");
        break;
      case 0x10EDC03F: // Color - blue
        repeatable = false;
          setAction("salon/leds", "color 10 10 255");
        break;
      case 0x10ED629D: // Action - on
        repeatable = false;
          setAction("salon/leds", "on");
        break;
      case 0x10ED22DD: // Action - snake
        repeatable = false;
          setAction("salon/leds", "snake");
        break;
      case 0x10ED20DF: // Action - rainbow
        repeatable = false;
          setAction("salon/leds", "arainbow");
        break;
      case 0x10EDE01F: // Action - off
        repeatable = false;
          setAction("salon/leds", "off");
        break;

      // Video projecteur

//      case 0x2F0: // On
//        repeatable = false;
//          setAction("salon/projecteur", "on");
//        break;
//      case 0x2F0: // Off
//        repeatable = false;
//        setAction("salon/projecteur", "off");
//        break;
      case 0x10ED9A65: // Toggle
        repeatable = false;
          setAction("salon/videoproj", "toggle");
        break;

      // Hi-Fi

      case 0x10EDAA55: // Toggle
        repeatable = false;
          setAction("salon/hifi", "toggle");
        break;

      // Kodi
      // Kodi addon : https://github.com/owagner/kodi2mqtt
      // Kodi python RPC : http://kodi.wiki/view/JSON-RPC_API#Python
//
//      case 0x2F0: // Menu
//        repeatable = true;
//        setAction("salon/kodi", "menu");
//        break;
//      case 0x2F0: // Up
//        repeatable = true;
//        setAction("salon/kodi", "up");
//        break;
//      case 0xAF0: // Down
//        repeatable = true;
//        setAction("salon/kodi", "down");
//        break;
//      case 0x2D0: // Left
//        repeatable = true;
//        setAction("salon/kodi", "left");
//        break;
//      case 0xCD0: // Right
//        repeatable = true;
//        setAction("salon/kodi", "right");
//        break;
//      case 0xD10: // Enter
//        repeatable = false;
//        setAction("salon/kodi", "return");
//        break;
//      case 0x9B0: // Play/Pause
//        repeatable = false;
//        setAction("salon/kodi", "play_pause");
//        break;
//      case 0x9B0: // Stop
//        repeatable = false;
//        setAction("salon/kodi", "stop");
//        break;
//      case 0x1B0: // Rewind
//        repeatable = false;
//        setAction("salon/kodi", "rewind");
//        break;
//      case 0x7B0: // Fast Forward
//        repeatable = false;
//        setAction("salon/kodi", "forward");
//        break;
//      case 0x7B0: // Volume up
//        repeatable = false;
//        setAction("salon/kodi", "volume_up");
//        break;
//      case 0x7B0: // Volume down
//        repeatable = false;
//        setAction("salon/kodi", "volume_down");
//        break;

      // Non-matching IR
      default:
        repeatable = false;
        break;
    }

    // Receive the next value
    delay(25);
    irrecv.resume();
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
  // Show something
  blink(50, 10);
}
