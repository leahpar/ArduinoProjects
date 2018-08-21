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
