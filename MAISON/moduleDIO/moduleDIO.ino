#include <constants.h>

// --- WIFI --------------------------------------------------------------------

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h> // For multiple wifi networks
ESP8266WiFiMulti wifiMulti;

// --- Serveur HTTP ------------------------------------------------------------

#include <ESP8266WebServer.h>
ESP8266WebServer server(80);

// --- Connexion serveur maison ------------------------------------------------

WiFiClient client;
const char* host = "10.0.0.44";
const int port = 80;
const char* url_cmd = "/Automa/AA_Groupe.php";
unsigned long cmd_debounce = millis();

// --- Radio -------------------------------------------------------------------

#include <DiOremote.h>

const int RX_PIN = D2;
const int TX_PIN = D1;

// --- HomeEasy protocol parameters ---
#define DiOremote_PULSEIN_TIMEOUT 1000000
#define DiOremote_FRAME_LENGTH 64
// Homeasy start lock : 2725 us (+/- 175 us)
#define DiOremote_START_TLOW 2550
#define DiOremote_START_THIGH 2900
// Homeeasy 0 : 310 us
#define DiOremote_0_TLOW 250
#define DiOremote_0_THIGH 450
//Homeeasy 1 : 1300 us
#define DiOremote_1_TLOW 1250
#define DiOremote_1_THIGH 1500

unsigned long codeRx = 0; // Received radio signal
int codeLen;
byte currentBit, previousBit;

DiOremote dioRemote = DiOremote(TX_PIN);

// --- codes radio -------------------------------------------------------------

const unsigned long CODES[][2]  = {
  // Groupe A
  { 1404155792, 1404155776 },
  { 1404155793, 1404155777 },
  { 1404155794, 1404155778 },
  { 1404155795, 1404155779 },
  // Groupe B
  { 1404155796, 1404155780 },
  { 1404155797, 1404155781 },
  { 1404155798, 1404155782 },
  { 1404155799, 1404155783 },
  // Groupe C
  { 1404155800, 1404155784 },
  { 1404155801, 1404155785 },
  { 1404155802, 1404155786 },
  { 1404155803, 1404155787 },
  // Groupe D
  { 1404155804, 1404155788 }, // Volets RDC
  { 1404155805, 1404155789 }, // Volets étage
  { 1404155806, 1404155790 },
  { 1404155807, 1404155791 },
  // Groupe G
  { 1404155824, 1404155808 }
};


//==============================================================================
// SETUP
//==============================================================================
void setup() {
  Serial.begin(74880);
  while (!Serial);

  pinMode(RX_PIN, INPUT);
  pinMode(TX_PIN, OUTPUT);

  Serial.println("------------------");
  Serial.println(
    "Version: "
    + String(__DATE__)
    + " - "
    + String(__TIME__));

  // Wifi

  Serial.println("Connecting ...");
  WiFi.mode(WIFI_STA);
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

  Serial.println("Setup done");
  Serial.println("------------------");
}

//==============================================================================
// LOOP
//==============================================================================
void loop(void) {

  // WIFI ==> Radio
  server.handleClient();

  // Radio ==> WIFI
  handleRadio();

  delay(10);
}

//==============================================================================
// READ RADIO
//==============================================================================
bool readRadio() {
  // Wait for incoming bit
  unsigned long t = pulseIn(RX_PIN, LOW, DiOremote_PULSEIN_TIMEOUT);

  // Only decypher from 2nd Homeeasy lock
  if (t > DiOremote_START_TLOW && t < DiOremote_START_THIGH) {

    for (codeLen = 0 ; codeLen < DiOremote_FRAME_LENGTH ; codeLen++) {
      // Read each bit (64 times)
      t = pulseIn(RX_PIN, LOW, DiOremote_PULSEIN_TIMEOUT);

      // Identify current bit based on the pulse length
      if (t > DiOremote_0_TLOW && t < DiOremote_0_THIGH)
        currentBit = 0;
      else if (t > DiOremote_1_TLOW && t < DiOremote_1_THIGH)
        currentBit = 1;
      else
        break;

      // If bit count is even, check Manchester validity & store in code
      if (codeLen % 2) {

        // Code validity verification (Manchester complience)
        if (!(previousBit ^ currentBit))
          break;

        // Store new bit
        codeRx <<= 1;
        codeRx |= previousBit;
      }
      previousBit = currentBit;
    }
  }

  if (codeLen == DiOremote_FRAME_LENGTH) {
    return true;
  }

  return false;
}

//==============================================================================
// HANDLE RADIO
// Check for radio sigal
//==============================================================================
void handleRadio() {

  // If signal is available
  if (readRadio()) {

    // Get cmd from signal
    String cmd = getCmd(codeRx);

    // Reset signal
    codeLen = 0;

    // Debounce 1sec
    if (millis() - cmd_debounce < 1000) return;

    cmd_debounce = millis();

    Serial.println("Received (radio): " + String(codeRx) + " => " + cmd);

    if (cmd != "NONE") {
      // Send command
      sendCommand(cmd);
    }
  }
}

//==============================================================================
// Send http command
//==============================================================================
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
// Get radio code from command
//==============================================================================
unsigned long getCode(String id, String cmd) {
  int c = cmd == "on" ? 0 : 1;

  if (id == "G") {
    return CODES[16][c];
  }
  else {
    int dio = 4 * ((int)(id.charAt(0)) - (int)'A')
                +  (int)(id.charAt(1)) - (int)'0' - 1;
    return CODES[dio][c];
  }
}

//==============================================================================
// Get command from radio code
//==============================================================================
String getCmd(unsigned long code) {
  // D1 OFF
  if (code == CODES[12][0]) return "?cmd=up&etage=2";
  // D1 ON
  if (code == CODES[12][1]) return "?cmd=down&etage=2";
  // D2 ON
  if (code == CODES[13][0]) return "?cmd=up&etage=3";
  // D2 OFF
  if (code == CODES[13][1]) return "?cmd=down&etage=3";
  // none
  return "NONE";
}

//==============================================================================
// HANDLE ROOT
// Gère les requêtes http
//==============================================================================
void handleRoot() {
  int ret;

  // Paramètres obligatoires
  if (!server.hasArg("cmd") || !server.hasArg("dio")) {
    handleHelp();
    return;
  }

  digitalWrite(LED_BUILTIN, LOW); // LED ON

  unsigned long c = getCode(server.arg("dio"), server.arg("cmd"));
  Serial.println(
    "Received (wifi): "
     + server.arg("dio") + " "
     + server.arg("cmd") + " "
     + " => "
     + String(c)
   );
  if (c > 0) {
    dioRemote.send(c);
    ret = 200;
  }
  else {
    ret = 404;
  }

  server.send(ret, "text/plain", "OK");

  digitalWrite(LED_BUILTIN, HIGH); // LED OFF

}

//==============================================================================
// HANDLE HELP
// Affiche l'usage
//==============================================================================
void handleHelp() {
  String help = "";
  help += "Version: " + String(__DATE__) + " - " + String(__TIME__);
  help += "\n";
  help += "Usage:\n";
  help += "http://" + WiFi.localIP().toString() + "/?cmd=<on|off>&dio=<([A-D][1-4])|G>\n";
  help += " [dio] : ID dio (A1, A2...)\n";
  help += " [cmd] : commande (on, off)\n";
  help += "\n";
  help += "Wifi:\n";
  help += " SSID: "+ WiFi.SSID() +"\n";
  help += " MAC:  "+ WiFi.macAddress() +"\n";
  help += " IP:   "+ WiFi.localIP().toString() +"\n";
  help += " RSSI: "+ String(WiFi.RSSI()) + " dB\n";
  help += "\n";

  server.send(400, "text/plain", help);
}
