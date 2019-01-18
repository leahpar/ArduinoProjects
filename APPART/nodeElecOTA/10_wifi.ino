#include <ESP8266WiFi.h>
#include <constants.h>

const char* _ssid = WIFI_SSID;
const char* _password = WIFI_PASSWD;

void wifi_setup() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(_ssid, _password);
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }
}


/*
// connect to wifi – returns true if successful or false if not
boolean connectWifi(){
  boolean state = true;
  int i = 0;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (i > 10){
      state = false;
      break;
    }
    i++;
  }
  return state;
}
*/