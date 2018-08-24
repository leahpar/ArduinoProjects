#include <constants.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <functional>
#include "switch.h"
#include "UpnpBroadcastResponder.h"
#include "CallbackFunction.h"

// prototypes
boolean connectWifi();

//on/off callbacks
bool switchChambreOn();
bool switchChambreOff();
bool switchSalonOn();
bool switchSalonOff();


const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWD;

boolean wifiConnected = false;

UpnpBroadcastResponder upnpBroadcastResponder;

Switch *chambre = NULL;
Switch *salon = NULL;
Switch *cinema = NULL;

void setup() {
  Serial.begin(74880);

  // Initialise wifi connection
  wifiConnected = connectWifi();

  if (wifiConnected) {
    upnpBroadcastResponder.beginUdpMulticast();

    int port = 80;
    // Define your switches here. Max 10
    // Format: Alexa invocation name, local port no, on callback, off callback
    chambre = new Switch("chambre", "lumière chambre", port++, switchCallback);
    salon   = new Switch("salon",   "lumière salon",   port++, switchCallback);
    cinema  = new Switch("cinema",  "cinéma",          port++, switchCallback);

    Serial.println("Adding switches upnp broadcast responder");
    upnpBroadcastResponder.addDevice(*chambre);
    upnpBroadcastResponder.addDevice(*salon);
    upnpBroadcastResponder.addDevice(*cinema);
  }
}

void loop() {
	 if (wifiConnected) {
      upnpBroadcastResponder.serverLoop();

      chambre->serverLoop();
      salon->serverLoop();
      cinema->serverLoop();
	 }
}

bool switchCallback(String device_id, String state) {
    Serial.println("Switch " + device_id + " turn " + state + " ...");
    return state == "ON";
}

// connect to wifi – returns true if successful or false if not
boolean connectWifi() {
  boolean state = true;
  int i = 0;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.println("Connecting to WiFi");

  // Wait for connection
  Serial.print("Connecting ...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (i > 10){
      state = false;
      break;
    }
    i++;
  }

  if (state){
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.println("");
    Serial.println("Connection failed.");
  }

  return state;
}
