#include <constants.h>

// --- WIFI --------------------------------------------------------------------

#include <ESP8266WiFi.h>
const char* wifi_ssid = WIFI_SSID;
const char* wifi_pass = WIFI_PASSWD;

// --- MQTT --------------------------------------------------------------------

#include <PubSubClient.h>
const char* mqtt_host = MQTT_SERVER;
const int   mqtt_port = MQTT_PORT;
const char* mqtt_user = MQTT_USER;
const char* mqtt_pwd  = MQTT_PWD;
const char* mqtt_client_id = "ArduinoProjo";
const char* mqtt_topic_video = "salon/videoproj";
const char* mqtt_topic_hours = "sensor/videoproj/salon";
const char* mqtt_topic_status = "status/videoproj";
WiFiClient wifiClient;
PubSubClient client(wifiClient);


// --- RS232 -------------------------------------------------------------------

#include <SoftwareSerial.h>
#define rxPin D5
#define txPin D6
SoftwareSerial rs232 =  SoftwareSerial(rxPin, txPin);

// --- DIVERS ------------------------------------------------------------------

int delayCommand   = 2000;
int delayToggle    = 20000;
int timeoutCommand = 10000;

unsigned long chrono;
unsigned long chronoTimeout = 0;
unsigned long chronoToggle  = 0;
int lampHours = 0;


// --- MACHINE STATE -----------------------------------------------------------

enum MachineStates {START, CONNECT_WIFI, CONNECT_MQTT,  UNKNOWN,
                    COMMAND_ON, PROJECTOR_ON, COMMAND_OFF, PROJECTOR_OFF,
                    LAMP_HOURS};
MachineStates machineState = START;

enum MQTTCommands {MQTT_NONE, MQTT_ON, MQTT_OFF, MQTT_TOGGLE, MQTT_STATE, MQTT_HOURS};
MQTTCommands mqttCommand = MQTT_NONE;

enum ProjectorCommands {PROJO_NONE, PROJO_ON, PROJO_OFF, PROJO_STATE, PROJO_HOURS};
ProjectorCommands projectorCommand = PROJO_NONE;



//==============================================================================
// THE MACHINE STATE
//==============================================================================
void videoMachine() {
  switch (machineState) {

    case START:
      setMachineState(CONNECT_WIFI, __LINE__);
      break;

    case CONNECT_WIFI:
      if (connectWifi()) {
        setMachineState(CONNECT_MQTT, __LINE__);
      }
      break;

    case CONNECT_MQTT:
      if (connectMqtt()) {
        setMachineState(UNKNOWN, __LINE__);
      }
      break;

    case UNKNOWN:
      chrono = 0;
      chronoToggle = millis();
      if (readProjectorStatus()) {
        setMachineState(PROJECTOR_ON, __LINE__);
      }
      else {
        setMachineState(PROJECTOR_OFF, __LINE__);
      }
      break;

    case LAMP_HOURS:
      sendLampHours();
      setMachineState(UNKNOWN, __LINE__);
      break;

    case PROJECTOR_ON:
      switch (mqttCommand) {
        case MQTT_OFF:
        case MQTT_TOGGLE:
          setMachineState(COMMAND_OFF, __LINE__);
          chrono = 0;
          chronoTimeout = millis();
          break;
        case MQTT_ON:
          mqttCommand = MQTT_NONE;
          break;
      }
      break;

    case PROJECTOR_OFF:
      switch (mqttCommand) {
        case MQTT_ON:
        case MQTT_TOGGLE:
          setMachineState(COMMAND_ON, __LINE__);
          chrono = 0;
          chronoTimeout = millis();
          break;
        case MQTT_OFF:
          mqttCommand = MQTT_NONE;
          break;
      }
      break;

    case COMMAND_ON:
      mqttCommand = MQTT_NONE;
      if (millis() - chrono > delayCommand) {
          chrono = millis();
          if (readProjectorStatus()) {
            readLampHours();
            sendProjectorStatus();
            projectorCommand = PROJO_NONE;
            setMachineState(LAMP_HOURS, __LINE__);
          }
          else {
            projectorCommand = PROJO_ON;
            callProjectorCommand();
          }
        }
      else if (millis() - chronoTimeout > timeoutCommand) {
          projectorCommand = PROJO_NONE;
          setMachineState(UNKNOWN, __LINE__);
        }
      break;

    case COMMAND_OFF:
      mqttCommand = MQTT_NONE;
      if (millis() - chrono > delayCommand) {
        chrono = millis();
        if (!readProjectorStatus()) {
          sendProjectorStatus();
          projectorCommand = PROJO_NONE;
          setMachineState(LAMP_HOURS, __LINE__);
        }
        else {
          readLampHours();
          projectorCommand = PROJO_OFF;
          callProjectorCommand();
        }
      }
      else if (millis() - chronoTimeout > timeoutCommand) {
        projectorCommand = PROJO_NONE;
        setMachineState(UNKNOWN, __LINE__);
      }
      break;
  }
}

//==============================================================================
// SET NEW MACHINE STATE
//==============================================================================
void setMachineState(MachineStates state, int line) {
  Serial.println(
      String("[")
    + String((int)(millis()/1000))
    + "ms;l." + String(line) + ") "
    + "]"
    + getMachineStateString(machineState)
    + " => "
    + getMachineStateString(state)
  );
  machineState = state;
}

//==============================================================================
// GET MACHINE STATE IN STRING
//==============================================================================
String getMachineStateString(MachineStates state) {
  switch (state) {
    case START:           return String("START");
    case CONNECT_WIFI:    return String("CONNECT_WIFI");
    case CONNECT_MQTT:    return String("CONNECT_MQTT");
    case UNKNOWN:         return String("UNKNOWN");
    case LAMP_HOURS:      return String("LAMP_HOURS");
    case PROJECTOR_ON:    return String("PROJECTOR_ON");
    case PROJECTOR_OFF:   return String("PROJECTOR_OFF");
    case COMMAND_ON:      return String("COMMAND_ON");
    case COMMAND_OFF:     return String("COMMAND_OFF");
  }
}

//==============================================================================
// MQTT CALLBACK
//==============================================================================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String action = String((char*)payload);

  Serial.println("MQTT[" + String(topic) + "]: " + action);
  blink(30, 0);

  if (action == "on") {
    if (millis() - chronoToggle > delayToggle) {
      mqttCommand = MQTT_ON;
      chronoToggle = millis();
    }
  }
  else if (action == "off") {
    if (millis() - chronoToggle > delayToggle) {
      mqttCommand = MQTT_OFF;
      chronoToggle = millis();
    }
  }
  else if (action == "toggle") {
    if (millis() - chronoToggle > delayToggle) {
      mqttCommand = MQTT_TOGGLE;
      chronoToggle = millis();
    }
  }
}

//==============================================================================
// READ STATE FROM PROJECTOR
//==============================================================================
bool readProjectorStatus() {
  projectorCommand = PROJO_STATE;
  int state = callProjectorCommand();
  projectorCommand = PROJO_NONE;
  return (state == 1);
}

//==============================================================================
// SEND VIDEOPROJECTOR STATUS TO MQTT
//==============================================================================
void sendProjectorStatus() {
  client.publish(
    mqtt_topic_status, machineState == PROJECTOR_ON ? "on" : "off",
    true
  );
}

//==============================================================================
// READ LAMP HOURS FROM PROJECTOR
//==============================================================================
void readLampHours() {
  projectorCommand = PROJO_HOURS;
  lampHours = callProjectorCommand();
  projectorCommand = PROJO_NONE;
}

//==============================================================================
// SEND LAMP HOURS TO MQTT
//==============================================================================
void sendLampHours() {
  if (lampHours > 0) {
    client.publish(mqtt_topic_hours, String(lampHours).c_str());
  }
}

//==============================================================================
// CALL COMMAND ON PROJECTOR
//==============================================================================
int callProjectorCommand() {
  String response;
  switch (projectorCommand) {
    case PROJO_ON:
      response = command("* 0 IR 001\r");
      break;
    case PROJO_OFF:
      response = command("* 0 IR 002\r");
      break;
    case PROJO_STATE:
      // DEBUG
      if (machineState == COMMAND_ON) return 1;
      if (machineState == COMMAND_OFF)  return 0;
      // END DEBUG
      response = command("* 0 Lamp ?\r");
      if (response.indexOf("Lamp 1") >= 0)
        return 1;
      else
        return 0;
      break;
    case PROJO_HOURS:
      // DEBUG
      return 2000;
      // END DEBUG
      response = command("* 0 Lamp\r");
      String response = command("* 0 Lamp\r");
      return response.substring(response.indexOf(' ')+1).toInt(); // 0 if error
      break;
  }
  return 0;
}

//==============================================================================
// SEND RS232 COMMAND
//==============================================================================
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

  return res;
}

//==============================================================================
// CONNECT WIFI
//==============================================================================
bool connectWifi() {
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
  int i = 0;
  while (!client.connect(mqtt_client_id, mqtt_user, mqtt_pwd)) {
    blink(20, 50 + 5*i++);
  }
  client.subscribe(mqtt_topic_video);
  client.publish("hello/world", mqtt_client_id);
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

//==============================================================================
// SETUP
// - WiFi config
// - MQTT config & callback
// - Init RS232
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
  client.setCallback(mqttCallback);
  //connectMqtt();

  // --- RS232 -----------------------------------------------------------------

  //Serial.println("RS232 serial...");
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  rs232.begin(9600);
  while(!rs232);

  Serial.println("Setup done");

  setMachineState(START, __LINE__);
}

//==============================================================================
// LOOP
// - Check WiFi connection
// - Check MQTT connection
// - Check subscribed mqtt channels
// - Run the state machine
//==============================================================================
void loop() {

  // Check subscribed mqtt channels
  client.loop();

  // run the machine
  videoMachine();

  // Check WiFi
  if (WiFi.status() != WL_CONNECTED) {
    setMachineState(CONNECT_WIFI, __LINE__);
  }
  // Check MQTT
  else if (!client.connected()) {
    setMachineState(CONNECT_MQTT, __LINE__);
  }

  delay(50);
}
