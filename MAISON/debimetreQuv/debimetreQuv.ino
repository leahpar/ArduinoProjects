#include <constants.h>

// --- WIFI --------------------------------------------------------------------

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h> // For multiple wifi networks
ESP8266WiFiMulti wifiMulti;

// --- OTA ---------------------------------------------------------------------

#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

const char* ota_passwd = "admin";
const char* ota_host   = "debimetre-quv";

// --- Débimètre ---------------------------------------------------------------

const int PIN_INTERRUPT = D7;
volatile unsigned long pulseCount = 0;
volatile unsigned long pulseCountTot = 0;
uint32_t curTime;
unsigned long startTime=0;    // début du nouveau comptage
unsigned long pusleInterval = 1000; // 10 sec
unsigned long PPL = 500; // Pulses par Litre
float volumeTotal = 0.0;

//==============================================================================
// SETUP
//==============================================================================

void setup() {
  Serial.begin(74880);
  while (!Serial);

  pinMode(PIN_INTERRUPT, INPUT);
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
  
  //ota_setup();
  Serial.println("OTA OK");

  // Debimètre

  attachInterrupt(PIN_INTERRUPT, pulseInterrupt, FALLING);


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

  curTime = millis();
  if (curTime - startTime > pusleInterval) {
    digitalWrite(LED_BUILTIN, LOW);
    // PPL impulsions pour 1 litre
    float l = 1.0 * pulseCount / PPL; // <= L (depuis dernière mesure)
    volumeTotal += l;
    float q = 1000.0 * l / (curTime - startTime); // <= L/s
    pulseCount = 0;
    startTime = curTime;
    Serial.println(String("")
      //+ String(volumeTotal) + " L ; " 
      + String(l) + " L ; " 
      + String(q) + " L/s" 
      //+ " ; Pulses : " + String(pulseCount) + " / " + String(pulseCountTot)
    );
    digitalWrite(LED_BUILTIN, HIGH);
  }

  // Check wifi
  //connectWifi();

  //server.handleClient();

  // OTA
  //ArduinoOTA.handle();

  delay(10);
}



void pulseInterrupt()
{
  pulseCount++;
  pulseCountTot++;
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

