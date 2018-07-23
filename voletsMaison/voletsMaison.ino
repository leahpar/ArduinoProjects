#include <constants.h>

// --- WIFI --------------------------------------------------------------------

#include <ESP8266WiFi.h>
const char* wifi_ssid = WIFI_SSID;
const char* wifi_pass = WIFI_PASSWD;

// --- Serveur HTTP ----------------------------------------------------------

#include <ESP8266WebServer.h>
ESP8266WebServer server(80);


const int PIN_ON_OFF = 3;
const int PIN_UP_DOWN = 4;
const int DELAY_RELAY = 150;
unsigned long command_start = 0;
const unsigned long MAX_TIMEOUT = 30 * 1000; // 30sec
unsigned long command_timeout = MAX_TIMEOUT;
bool stop = true;

//==============================================================================
// SETUP
//==============================================================================
void setup() {
  Serial.begin(74880);
  while (!Serial);

  pinMode(PIN_ON_OFF, OUTPUT);
  pinMode(PIN_UP_DOWN, OUTPUT);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // Wifi

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_pass);
  Serial.println(WiFi.macAddress());

  // HTTP server

  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");

}

//==============================================================================
// LOOP
//==============================================================================
void loop(void) {
  server.handleClient();

  if (!stop && millis() - command_start  > command_timeout) {
    commandStop();
  }

  delay(10);
}

//==============================================================================
// HANDLE ROOT
// Gère les requêtes http
// format : /?d=<up|down|stop>[&t=<xxx>]
//==============================================================================
void handleRoot() {

  // Paramètre "d" obligatoire
  if (!server.hasArg("d")) {
    server.send(400, "text/plain", "/?d=<up|down|stop>[&t=<xxx>]");
  }

  digitalWrite(LED_BUILTIN, LOW);

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
  if (server.arg("d") == "down") {
    commandDown();
  }
  else {
    commandStop();
  }

  // Réponse http 200 - OK
  server.send(200, "text/plain", "OK");

  digitalWrite(LED_BUILTIN, HIGH);

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
