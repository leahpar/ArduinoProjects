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
const char* mqtt_client_id = "ArduinoProjo";
const char* mqtt_topic_video = "salon/videoproj";
//const char* mqtt_topic_minix = "salon/minix";
const char* mqtt_topic_hours = "sensor/videoproj/salon";
WiFiClient wifiClient;
PubSubClient client(wifiClient);

// --- RS232 ---------------------------------------------

#include <SoftwareSerial.h>
#define rxPin D5
#define txPin D6
SoftwareSerial rs232 =  SoftwareSerial(rxPin, txPin);
bool videoproj_on = false;
bool b_command_on = false;
unsigned long t_command_on;
unsigned long t_previous_command_on;
bool b_command_off = false;
unsigned long t_command_off;
unsigned long t_previous_command_off;
unsigned long t_videoproj_toggle;

// --- INFRAROUGE ----------------------------------------

/*
#include <IRremoteESP8266.h>
#include <IRsend.h>
#define IR_LED 4  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
IRsend irsend(IR_LED);
*/

// --- DIVERS --------------------------------------------

unsigned long t_current;
unsigned long t_previous_wifi_check;
unsigned long t_previous_lamp_hours;


void setup() {

  Serial.begin(74880);
  while (!Serial);
  Serial.println("Start");
  
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH); // led off

  // side led
  //pinMode(D7, OUTPUT);
  //digitalWrite(D7, LOW);

  // --- WIFI ---------------------------------------------

  WiFi.mode(WIFI_STA); // mode standard
  WiFi.begin(wifi_ssid, wifi_pass);
  reconnect_wifi();
  
  // --- MQTT ---------------------------------------------
  
  client.setServer(mqtt_host, mqtt_port);
  client.setCallback(mqtt_callback);
  reconnect_mqtt();

  // --- RS232 ---------------------------------------------

  Serial.println("RS232 serial...");
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  rs232.begin(9600);
  while(!rs232);

  // --- INFRAROUGE ----------------------------------------
  /*
  irsend.begin();
  */
  

  Serial.println("Setup done");

  t_current  = millis();
  t_previous_wifi_check = millis();
  t_previous_lamp_hours = 0;
  t_command_on = 0;
  t_command_off = 0;
  t_previous_command_on = 0;
  t_previous_command_off = 0;
  
  blink(100,100);
  blink(100,100);
  blink(100,100);
  //delay(500);
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
    client.subscribe(mqtt_topic_video);
    //client.subscribe(mqtt_topic_minix);

    client.publish("hello/world", mqtt_client_id);
  }
}


void loop() {

  t_current = millis();

  if ((unsigned long)(t_current - t_previous_wifi_check) >= 10000) {
    // Every 10 seconds, check wifi & mqtt connections
    t_previous_wifi_check = t_current;
    // blink(20, 50);
    reconnect_wifi();
    reconnect_mqtt();
  }
  
  // Check for command
  if (b_command_on && (unsigned long)(t_current - t_previous_command_on) >= 2000) {
    t_previous_command_on = millis();
    command_on();
  }
  if (b_command_off && (unsigned long)(t_current - t_previous_command_off) >= 2000) {
    t_previous_command_off = millis();
    command_off();
  }
  
  // Check subscribed mqtt channels
  client.loop();

  delay(50);
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String action;
 
  payload[length] = '\0';
  String message = String((char*)payload);
  
  action = message;

  Serial.println("[" + String(topic) + "] " + message);
  blink(20,50);
  blink(20,50);
  blink(20,50);

  if (action == "off") {
    turn_off();
  }
  else if (action == "on") {
    turn_on();
  }
  else if (action == "toggle") {
    if ((unsigned long)(millis() - t_videoproj_toggle) > 5000) {
      t_videoproj_toggle = millis();

      if (get_status()) {
        turn_off();
      }
      else {
        turn_on();
      }
    }
  }
  else if (action == "status") {
    get_status();
  }
  else if (action == "hours") {
    send_lamp_hours();
  }
  
  /*
  else if (String(topic) == mqtt_topic_minix) {
    if (action == "toggle") {
      irsend.sendNEC(0x807F18E7, 32);
      Serial.println("SEND NEC");
    }
  }
  */
}

void send_lamp_hours() {
  if (videoproj_on) {
    if ((unsigned long)(t_current - t_previous_lamp_hours) >= 10000) {
      t_previous_lamp_hours = millis();
      String response = command("* 0 Lamp\r");
      response = response.substring(response.indexOf(' ')+1);
      Serial.println(response);
      client.publish(mqtt_topic_hours, response.c_str());  
    }
    else {
      Serial.println("too soon");
    }
  }
}


void turn_on() {
  b_command_on = true;
  b_command_off = false;
  t_command_on = millis();
}
void turn_off() {
  b_command_on = false;
  b_command_off = true;
  t_command_off = millis();
}

void command_on() {
  Serial.println("Videoproj on");

  if (get_status()) {
    b_command_on = false;
    b_command_off = false;
    // send lamp hours
    send_lamp_hours();
    Serial.println("End command");
    Serial.println();
  }
  else {
    command("* 0 IR 001\r");
  }

  if ((unsigned long)(millis() - t_command_on) > 15000) {
    b_command_on = false;
    Serial.println("Command timeout");
    Serial.println();
  }
}

void command_off() {
  Serial.println("Videoproj off");

  if (get_status()) {
    // send lamp hours
    send_lamp_hours();
  
    command("* 0 IR 002\r");
  }
  else {
    Serial.println("End command");
    Serial.println();
    b_command_on = false;
    b_command_off = false;
  }

  if ((unsigned long)(millis() - t_command_off) > 10000) {
    b_command_off = false;
    Serial.println("Command timeout");
    Serial.println();
  }
}

boolean get_status() {
  String response = command("* 0 Lamp ?\r");
  if (response.indexOf("Lamp 1") >= 0)
    videoproj_on = true;
  else
    videoproj_on = false;
  Serial.println(response);
  return videoproj_on;
}

String command(char* c) {
  String res = "";
  char inByte;
  Serial.println(String("=> ") + c);
  
  // empty rs232 data
  while (rs232.available()) rs232.read();
  
  // Send comamnd
  rs232.write(c);
  
  // Wait for response
  delay(200);
    
  // Read response
  while (rs232.available()) {
    inByte = rs232.read();
    if (inByte == '\r') inByte = ' ';
    //Serial.write(inByte);
    res = res + inByte;
  }
  res = res;
  //Serial.println(String("res = ") + res);
  return res;
}

void blink(int up, int down) {
  digitalWrite(BUILTIN_LED, LOW);
  delay(up);
  digitalWrite(BUILTIN_LED, HIGH);
  delay(down);
}



