#include <constants.h>

// --- WIFI --------------------------------------------------------------------

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h> // For multiple wifi networks
ESP8266WiFiMulti wifiMulti;

// --- Serveur HTTP ------------------------------------------------------------

#include <ESP8266WebServer.h>
ESP8266WebServer server(80);

// --- Divers ------------------------------------------------------------------

const int PIN_ON_OFF = D1;
const int PIN_UP_DOWN = D2;
const int DELAY_RELAY = 150;
unsigned long command_start = 0;
const unsigned long MAX_TIMEOUT = 30 * 1000; // 30sec
unsigned long command_timeout = MAX_TIMEOUT;
bool stop = true;

// --- Commande générale -------------------------------------------------------

const int PIN_CMD_BUTTON_1 = D5;
const int PIN_CMD_BUTTON_2 = D6;
int pin_cmd_button_1_state;
int pin_cmd_button_2_state;
unsigned long debounce_timer;
WiFiClient client;
const char* host = "10.0.0.44";
const int port = 80;
const char* url_cmd = "/volets/?cmd=";

//==============================================================================
// SETUP
//==============================================================================
void setup() {
  Serial.begin(74880);
  while (!Serial);

  pinMode(PIN_ON_OFF, OUTPUT);
  digitalWrite(PIN_ON_OFF, LOW);

  pinMode(PIN_UP_DOWN, OUTPUT);
  digitalWrite(PIN_UP_DOWN, LOW);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  pinMode(PIN_CMD_BUTTON_1, INPUT_PULLUP);
  pinMode(PIN_CMD_BUTTON_2, INPUT_PULLUP);

  // Wifi

  Serial.println("Connecting ...");

  WiFi.mode(WIFI_STA);
  /*
  WiFi.begin(wifi_ssid, wifi_pass);
  */
  wifiMulti.addAP(MULTI_WIFI_1);
  wifiMulti.addAP(MULTI_WIFI_2);

  Serial.print("mac address : ");
  Serial.println(WiFi.macAddress());

  int i = 0;
  // Wait for the Wi-Fi to connect:
  // scan for Wi-Fi networks, and connect to the strongest network
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print('.');
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
  }
  Serial.println();
  Serial.print("Connected to : ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address : ");
  Serial.println(WiFi.localIP());

  // HTTP server

  server.on("/", handleRoot);
  server.onNotFound(handleHelp);
  server.begin();
  Serial.println("HTTP server started");

  // Pins
  pin_cmd_button_1_state = 1;
  pin_cmd_button_2_state = 1;
  debounce_timer = millis();
}

//==============================================================================
// LOOP
//==============================================================================
void loop(void) {
  server.handleClient();

  if (!stop && millis() - command_start  > command_timeout) {
    commandStop();
  }

  if (pinup(PIN_CMD_BUTTON_1, pin_cmd_button_1_state)) {
    Serial.println("PIN_CMD_BUTTON_1 UP");
  }
  if (pinup(PIN_CMD_BUTTON_2, pin_cmd_button_2_state)) {
    Serial.println("PIN_CMD_BUTTON_2 UP");
  }
  if (pindown(PIN_CMD_BUTTON_1, pin_cmd_button_1_state)) {
    Serial.println("PIN_CMD_BUTTON_1 DOWN");
    sendCommand("up");
  }
  if (pindown(PIN_CMD_BUTTON_2, pin_cmd_button_2_state)) {
    Serial.println("PIN_CMD_BUTTON_2 DOWN");
    sendCommand("down");
  }

  delay(10);
}

bool pindown(int pin, int &state) {
  if (state == 1
    && digitalRead(pin) == 0
    && millis() - debounce_timer > 1000) {
    state = 0;
    debounce_timer = millis();
    return true;
  }
  return false;
}
bool pinup(int pin, int &state) {
  if (state == 0
    && digitalRead(pin) == 1
    && millis() - debounce_timer > 1000) {
    state = 1;
    debounce_timer = millis();
    return true;
  }
  return false;
}

void sendCommand(String cmd) {
  if (!client.connect(host, port)) {
    Serial.println("connection failed");
    return;
  }
  client.print(String("GET ") + url_cmd + cmd + " HTTP/1.1\r\n" +
             "Host: " + host + "\r\n" +
             "Connection: close\r\n\r\n");
}

//==============================================================================
// HANDLE HELP
// Affiche l'usage
//==============================================================================
void handleHelp() {
  String help;
  help  = "Usage:\n";
  help += "http://" + WiFi.localIP().toString() + "/?d=<up|down|stop>&t=<xx>\n";
  help += " [d] : direction (obligatoire) => up, down, stop\n";
  help += " [t] : temps (facultatif) => en ms\n";
  help += "       defaut = " + String(MAX_TIMEOUT) + " ms\n";
  help += "\n";
  help += "Wifi:\n";
  help += " SSID: "+ WiFi.SSID() +"\n";
  help += " MAC:  "+ WiFi.macAddress() +"\n";
  help += " IP:   "+ WiFi.localIP().toString() +"\n";
  help += " RSSI: "+ String(WiFi.RSSI()) + " dB\n";

  server.send(400, "text/plain", help);
}

//==============================================================================
// HANDLE ROOT
// Gère les requêtes http
//==============================================================================
void handleRoot() {

  // Paramètre "d" obligatoire
  if (!server.hasArg("d")) {
    handleHelp();
    return;
  }

  digitalWrite(LED_BUILTIN, LOW); // LED ON

  // TImeout à MAX_TIMEOUT par défaut
  command_timeout = MAX_TIMEOUT;

  // Paramètre "t"
  if (server.hasArg("t")) {
    command_timeout = atol(server.arg("t").c_str());
  }

  // On lance la commande du volet
  command_start = millis();
  if (server.arg("d") == "up") {
    commandUp();
  }
  else if (server.arg("d") == "down") {
    commandDown();
  }
  else {
    commandStop();
  }

  // Réponse http 200 - OK
  server.send(
    200,
    "text/plain",
    "OK " + server.arg("d") + " " + command_timeout
  );

  digitalWrite(LED_BUILTIN, HIGH); // LED OFF

}


//==============================================================================
// COMMAND UP
// Commande d'ouverture du volet
//==============================================================================
void commandUp() {
  Serial.println("UP (" + String(command_timeout) + ")");
  stop = false;
  digitalWrite(PIN_ON_OFF, LOW);
  delay(DELAY_RELAY);
  digitalWrite(PIN_UP_DOWN, HIGH);
  delay(DELAY_RELAY);
  digitalWrite(PIN_ON_OFF, HIGH);
  delay(DELAY_RELAY);
}

//==============================================================================
// COMMAND DOWN
// Commande de fermeture du volet
//==============================================================================
void commandDown() {
  Serial.println("DOWN (" + String(command_timeout) + ")");
  stop = false;
  digitalWrite(PIN_ON_OFF, LOW);
  delay(DELAY_RELAY);
  digitalWrite(PIN_UP_DOWN, LOW);
  delay(DELAY_RELAY);
  digitalWrite(PIN_ON_OFF, HIGH);
  delay(DELAY_RELAY);
}

//==============================================================================
// COMMAND STOP
// Commande d'arrêt du volet
//==============================================================================
void commandStop() {
  stop = true;
  Serial.println("STOP");
  digitalWrite(PIN_ON_OFF, LOW);
  delay(DELAY_RELAY);
  digitalWrite(PIN_UP_DOWN, LOW);
  delay(DELAY_RELAY);
}
