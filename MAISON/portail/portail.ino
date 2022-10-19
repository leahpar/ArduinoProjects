//==============================================================================
// Module de commande du portail
// -----------------------------
// 2018-08-31 : Version 1.0 RB Ecriture
// 2018-09-07 : Version 1.1 FB Ecriture-traces dans la cde down
// 2018-09-10 : Version 1.2 RB Nettoyage de code
// 2022-10-15 : Version 1.0 RB Reprise du module volets
//==============================================================================

#define WIFI_SSID    "YOUR _SSID"
#define WIFI_PASSWD  "your_wifi_password"
#define MULTI_WIFI_1    WIFI_SSID, WIFI_PASSWD
//#include <constants.h>

// --- WIFI --------------------------------------------------------------------
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h> // For multiple wifi networks
ESP8266WiFiMulti wifiMulti;

// --- Serveur HTTP ------------------------------------------------------------
#include <ESP8266WebServer.h>
ESP8266WebServer server(80);

// --- Attribution des pins ----------------------------------------------------
const int PIN_CMD_OPEN  = D1;    // Commande OPEN
const int PIN_CMD_CLOSE = D2;    // Commande CLOSE
const int PIN_IN_SDA    = D5;    // Signal portail ouvert

// --- Divers ------------------------------------------------------------------
const int DELAY_CMD = 100;         // Durée de l'impulsion pour les commandes (ms)
volatile bool isClosed = false;    // Etat du portail
volatile int etat = 0;             // -1 = ouverture ; 0 = arrêté ; +1 = fermeture
volatile unsigned long last_sda = 0;

long time_to_stop = -1;

// --- Serveur central pour commande de groupe ---------------------------------
// TODO: utiliser des constantes
WiFiClient client;
//const char* host = "192.168.1.25";                // Adresse du serveur central
//const int port = 80;                              // Port du serveur
//const char* url_cmd = "/Automa/AA_groupe.php/?cmd=";   // Application pour le groupe de volets


//==============================================================================
// PULSE INTERRUPT
//==============================================================================
void ICACHE_RAM_ATTR pulseInterrupt() {
  if (etat == 0 && !isClosed) {
    etat = 1; // en cours de fermeture
  }
  else if (etat == 0 && isClosed) {
    etat = -1; // en cours d'ouverture
  }
  isClosed = false;
  last_sda = millis();
}

//==============================================================================
// SETUP
//==============================================================================
void setup() {
  Serial.begin(19200);
  while (!Serial);

  // --- Initialisation des pins -----------------------------------------------
  pinMode(PIN_CMD_OPEN, OUTPUT);
  digitalWrite(PIN_CMD_OPEN, LOW);

  pinMode(PIN_CMD_CLOSE, OUTPUT);
  digitalWrite(PIN_CMD_CLOSE, LOW);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);      // led off

  // --- Interrupts ------------------------------------------------------------
  pinMode(PIN_IN_SDA, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_IN_SDA), pulseInterrupt, FALLING);
  last_sda = millis();

  // --- Initialisation wifi ---------------------------------------------------

  Serial.println("Connecting...");

  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(MULTI_WIFI_1);

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

}

//==============================================================================
// LOOP
//==============================================================================
void loop(void) {

  // Vérifie la connexion wifi (et reconnecte si besoin)
  connectWifi();

  // Gère les requêtes http en attente
  server.handleClient();

  // Pas de mouvement depuis Xsec, le portail est arrêté
  noInterrupts();
  if (millis() - last_sda > 3000) {
    if (digitalRead(PIN_IN_SDA) == HIGH) {
      isClosed = true;
    }
    else {
      isClosed = false;
    }
    etat = 0;
    String xx = String(digitalRead(PIN_IN_SDA)); 
    xx += " ";
    xx += String(etat);
    xx += " ";
    xx += String(isClosed);
    xx += " ";
    xx += String(last_sda/1000);
    Serial.println(xx);
    last_sda = millis();
  }
  interrupts();

  if (isClosed) {
    digitalWrite(LED_BUILTIN, HIGH); // LED OFF
  } else {
      digitalWrite(LED_BUILTIN, LOW); // LED ON
  }

  // Il est temps de s'arrêter
  if (time_to_stop > 0 && millis() > time_to_stop) {
    commandStop();
    time_to_stop = -1;
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
//      Envoie de la commande de groupe au serveur
//==============================================================================
/*
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
*/

//==============================================================================
// HANDLE HELP
// Affiche l'usage
//==============================================================================
void handleHelp() {
  String help = "\n\n\n\n\n\n\n\n\n";

  help += "Usage:\n";
  help += "http://" + WiFi.localIP().toString() + "/?d=<open|close|stop>&t=<xx>\n";
  help += " [d] : direction (obligatoire) => open, close, stop\n";
  help += " [t] : temps (facultatif) => en ms\n";
  help += "\n";

  if (isClosed) {
    help += "Portail: FERME\n";
  } else {
    help += "Portail: OUVERT\n";
  }
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

  // --- Paramètre "t", 0 par défaut
  unsigned long t = 0;
  if (server.hasArg("t")) {
    t = atol(server.arg("t").c_str());
  }

  // --- On lance la commande du portail
  if (server.arg("d") == "open") {
    commandOpen(t);
  }
  else if (server.arg("d") == "close") {
    commandClose(t);
  }
  else {
    commandStop();
  }

  // --- et on répond:   http 200 - OK
  server.send(
    200,
    "text/plain",
    "OK " + server.arg("d") + " " + t
  );

  digitalWrite(LED_BUILTIN, HIGH); // LED OFF
}

//==============================================================================
// COMMAND STOP
// Commande d'arrêt du volet
//==============================================================================
void commandStop() {
  Serial.println("STOP");
  switch (etat) {
    case -1: // en cours d'ouverture
      digitalWrite(PIN_CMD_CLOSE, HIGH);
      delay(DELAY_CMD);
      digitalWrite(PIN_CMD_CLOSE, LOW);
      //etat = 0;
      break;
    case 1: // en cours de fermeture
      digitalWrite(PIN_CMD_OPEN, HIGH);
      delay(DELAY_CMD);
      digitalWrite(PIN_CMD_OPEN, LOW);
      //etat = 0;
      break;
    case 0: // Déjà arrêté
      break;
  }
}

//==============================================================================
// COMMAND OPEN
// Commande d'ouverture du portail
//==============================================================================
void commandOpen(unsigned long t) {
  Serial.println("OPEN (" + String(t) + ")");
  etat = -1; // en cours d'ouverture
  digitalWrite(PIN_CMD_OPEN, HIGH);
  delay(DELAY_CMD);
  digitalWrite(PIN_CMD_OPEN, LOW);

  if (t > 0) {
    time_to_stop = millis() + t;
  }
}

//==============================================================================
// COMMAND CLOSE
// Commande de fermeture du portail
//==============================================================================
void commandClose(unsigned long t) {
  Serial.println("CLOSE (" + String(t) + ")");
  etat = 1; // en cours de fermeture
  digitalWrite(PIN_CMD_CLOSE, HIGH);
  delay(DELAY_CMD);
  digitalWrite(PIN_CMD_CLOSE, LOW);

  if (t > 0) {
    time_to_stop = millis() + t;
  }
}

