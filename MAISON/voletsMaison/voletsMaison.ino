//==============================================================================
// Module de commande des volets
// -----------------------------
// 2018-08-31 : Version 1.  RB Ecriture
// 2018-09-07 : Version 1.1 FB Ecriture-traces dans la cde down
// 2018-09-10 : Version 1.2 RB Nettoyage de code
//==============================================================================
#include <constants.h>

// --- WIFI --------------------------------------------------------------------
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h> // For multiple wifi networks
ESP8266WiFiMulti wifiMulti;

// --- Serveur HTTP ------------------------------------------------------------
#include <ESP8266WebServer.h>
ESP8266WebServer server(80);

// --- Attribution des pins ----------------------------------------------------
const int PIN_ON_OFF = D1;     // Relai 1: phase oui/non
const int PIN_UP_DOWN = D2;    // Relai 2: montée/descente

// --- Divers ------------------------------------------------------------------
const int DELAY_RELAY = 150;      // Entre deux cdes des relais
unsigned long command_start = 0;
const unsigned long MAX_TIMEOUT = 30 * 1000; // 30sec
unsigned long command_timeout = MAX_TIMEOUT;
bool stop = true;

// --- Etats des relais: relai du courrant (phase) -----------------------------
#define ON LOW           // Coupe la phase du bouton et l'envoie sur le relai 2
#define OFF HIGH         // Renvoie la phase sur le bouton

// --- Etats des relais: relai de direction (vers le moteur) -------------------
#define UP LOW          // Envoie la phase sur le fils : montée
#define DOWN HIGH       // Envoie la phase sur le fils : descente

// --- Commande pour le groupe de volets ---------------------------------------
const int PIN_CMD_BUTTON_1 = D5;
const int PIN_CMD_BUTTON_2 = D6;
int pin_cmd_button_1_state;
int pin_cmd_button_2_state;
unsigned long debounce_timer;

// --- Serveur central pour commande de groupe ---------------------------------
WiFiClient client;
const char* host = "192.168.1.25";                // Adresse du serveur central
const int port = 80;                              // Port du serveur
const char* url_cmd = "/Automa/AA_groupe.php/?cmd=";   // Application pour le groupe de volets

//==============================================================================
// SETUP
//==============================================================================
void setup() {
  Serial.begin(74880);
  while (!Serial);

  // --- Initialisation des pins -----------------------------------------------
  pinMode(PIN_ON_OFF, OUTPUT);
  digitalWrite(PIN_ON_OFF, OFF);        // repos: laisse la phase vers le bouton

  pinMode(PIN_UP_DOWN, OUTPUT);
  digitalWrite(PIN_UP_DOWN, DOWN);      // repos:

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);      // led off

  pinMode(PIN_CMD_BUTTON_1, INPUT_PULLUP);
  pinMode(PIN_CMD_BUTTON_2, INPUT_PULLUP);

  // --- Initialisation wifi ---------------------------------------------------

  Serial.println("Connecting...");

  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(MULTI_WIFI_1);
  wifiMulti.addAP(MULTI_WIFI_2);

  Serial.print("mac address: ");
  Serial.println(WiFi.macAddress());

  connectWifi();

  String help = "";
  help += " SSID: " + WiFi.SSID() +"\n";
  help += " MAC:  " + WiFi.macAddress() +"\n";
  help += " IP:   " + WiFi.localIP().toString() +"\n";
  help += " RSSI: " + String(WiFi.RSSI()) + " dB\n";
  Serial.println(help);

  // --- Initialisation du serveur HTTP ----------------------------------------
  server.on("/", handleRoot);
  server.onNotFound(handleHelp);
  server.begin();
  Serial.println("HTTP server started");

  // --- Initialisations de l'état des boutons de groupe -----------------------
  pin_cmd_button_1_state = 1;
  pin_cmd_button_2_state = 1;
  debounce_timer = millis();
}

//==============================================================================
// LOOP
//==============================================================================
void loop(void) {

  // Vérifie la connexion wifi (et reconnecte si besoin)
  connectWifi();

  // Gère les requêtes http en attente
  server.handleClient();

  // Timeout des commandes
  if (!stop && millis() - command_start > command_timeout) {
    commandStop();
  }

  // Gestion des boutons de groupe
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

//==============================================================================
// Vérifie la connexion wifi (et reconnecte si besoin)
//==============================================================================
void connectWifi() {
  // Wait for the Wi-Fi to connect:
  // scan for Wi-Fi networks, and connect to the strongest network
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print('.');
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
  }
}

//==============================================================================
//      Bouton de groupe activé ?
//==============================================================================
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

//==============================================================================
//      Bouton de groupe relaché ?
//==============================================================================
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

//==============================================================================
//      Envoie de la commande de groupe au serveur
//==============================================================================
void sendCommand(String cmd) {
  if (!client.connect(host, port)) {
    Serial.println("Serveur central non connecté");
    return;
  }
  String Cde = String("GET ") + url_cmd + cmd;
  String Request = Cde + " HTTP/1.1\r\n"    // GET
                 + "Host: " + host + "\r\n" // headers
                 + "Connection: close\r\n"  // headers
                 + "\r\n";                  // end request
  client.print(Request);
  Serial.println("Cde groupée: " + Cde);
}

//==============================================================================
// HANDLE HELP
// Affiche l'usage
//==============================================================================
void handleHelp() {
  String help = "\n\n\n\n\n\n\n\n\n";

  help += "Usage:\n";
  help += "http://" + WiFi.localIP().toString() + "/?d=<up|down|stop>&t=<xx>\n";
  help += " [d] : direction (obligatoire) => up, down, stop\n";
  help += " [t] : temps (facultatif) => en ms\n";
  help += "          defaut = " + String(MAX_TIMEOUT) + " ms\n";
  help += "\n";

  help += "Wifi:\n";
  help += " SSID: "+ WiFi.SSID() +"\n";
  help += " MAC:   "+ WiFi.macAddress() +"\n";
  help += " IP:    "+ WiFi.localIP().toString() +"\n";
  help += " RSSI: "+ String(WiFi.RSSI()) + " dB\n";

  help += " Version: "+ String(__DATE__) + " - " + String(__TIME__) + "\n";

  server.send(400, "text/plain", help);
}

//==============================================================================
// HANDLE ROOT
// Gère les requêtes http
//==============================================================================
void handleRoot() {

  // --- Paramètre "d" obligatoire, sinon envoi de l'aide
  if (!server.hasArg("d")) {
    handleHelp();
    return;
  }

  digitalWrite(LED_BUILTIN, LOW); // LED ON

  // --- Paramètre "t", sinon Timeout à MAX_TIMEOUT par défaut
  command_timeout = MAX_TIMEOUT;
  if (server.hasArg("t")) {
    command_timeout = atol(server.arg("t").c_str());
  }

  // --- On lance la commande du volet
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

  // --- et on répond:   http 200 - OK
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
  digitalWrite(PIN_ON_OFF, OFF);
  Serial.println(" - PIN_ON_OFF, OFF");
  delay(DELAY_RELAY);
  digitalWrite(PIN_UP_DOWN, UP);
  Serial.println(" - PIN_UP_DOWN, UP");
  delay(DELAY_RELAY);
  digitalWrite(PIN_ON_OFF, ON);
  Serial.println(" - PIN_ON_OFF, ON");
  delay(DELAY_RELAY);
}

//==============================================================================
// COMMAND DOWN
// Commande de fermeture du volet
//==============================================================================
void commandDown() {
  Serial.println("DOWN (" + String(command_timeout) + ")");
  stop = false;
  digitalWrite(PIN_ON_OFF, OFF);
  Serial.println(" - PIN_ON_OFF, OFF");
  delay(DELAY_RELAY);
  digitalWrite(PIN_UP_DOWN, DOWN);
  Serial.println(" - PIN_UP_DOWN, DOWN");
  delay(DELAY_RELAY);
  digitalWrite(PIN_ON_OFF, ON);
  Serial.println(" - PIN_ON_OFF, ON");
  delay(DELAY_RELAY);
}

//==============================================================================
// COMMAND STOP
// Commande d'arrêt du volet
//==============================================================================
void commandStop() {
  stop = true;
  Serial.println("STOP");
  digitalWrite(PIN_ON_OFF, OFF);
  Serial.println(" - PIN_ON_OFF, OFF");
  delay(DELAY_RELAY);
  digitalWrite(PIN_UP_DOWN, DOWN);
  Serial.println(" - PIN_UP_DOWN, DOWN");
  delay(DELAY_RELAY);
}
