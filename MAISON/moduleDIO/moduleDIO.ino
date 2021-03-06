/*
http://charleslabs.fr/fr/project-Contr%C3%B4le+de+prises+DiO+avec+Arduino
*/

#include <constants.h>

// --- WIFI --------------------------------------------------------------------

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h> // For multiple wifi networks
ESP8266WiFiMulti wifiMulti;

// --- Serveur HTTP ------------------------------------------------------------

#include <ESP8266WebServer.h>
ESP8266WebServer server(80);

// --- OTA ---------------------------------------------------------------------

#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

const char* ota_passwd = "admin";
const char* ota_host   = "module-dio";


// --- Radio -------------------------------------------------------------------

//const int RX_PIN = D1;
const int TX_PIN = D7;

#include <DiOremote.h>
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

  //pinMode(RX_PIN, INPUT);
  pinMode(TX_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println("------------------");
  Serial.println(
    "Version: "
    + String(__DATE__)
    + " - "
    + String(__TIME__));

  // Wifi

  Serial.println("Connecting...");
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(MULTI_WIFI_1);
  wifiMulti.addAP(MULTI_WIFI_2);

  Serial.print("mac address : ");
  Serial.println(WiFi.macAddress());

  connectWifi();
  Serial.println("WIFI OK");

  // OTA
  
  ota_setup();
  Serial.println("OTA OK");

  // HTTP server

  server.on("/", handleRoot);
  server.onNotFound(handleHelp);
  server.begin();
  Serial.println("HTTP server started");

  Serial.println("Setup done");
  Serial.println("------------------");
}

//==============================================================================
// Check wifi connection
//==============================================================================
void connectWifi() {
  bool wasConnected = true;
  // Wait for the Wi-Fi to connect:
  // scan for Wi-Fi networks, and connect to the strongest network
  while (wifiMulti.run() != WL_CONNECTED) {
    wasConnected = false;
    Serial.print('.');
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
  }
  if (!wasConnected) {
    Serial.println();
    Serial.print("Connected to : ");
    Serial.println(WiFi.SSID());
    Serial.print("IP address : ");
    Serial.println(WiFi.localIP());
  }
}

//==============================================================================
// LOOP
//==============================================================================
void loop(void) {

  // Check wifi
  connectWifi();

  // WIFI ==> Radio
  server.handleClient();

  // OTA
  ArduinoOTA.handle();

  delay(10);
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

//==============================================================================
// UPDATE OTA
//==============================================================================

void ota_setup() {
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(ota_host);

  // No authentication by default
  ArduinoOTA.setPassword(ota_passwd);
  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating" + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    //digitalWrite(LED_BUILTIN, progress % 10 < 5 ? HIGH : LOW);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}

