#include <ESP8266WiFi.h>

unsigned long ntp_t = millis();


void setup() {
  Serial.begin(74880);
  Serial.println("Booting");
  
  wifi_setup();

  Debug.begin(_host, Debug.INFO);
  Debug.setSerialEnabled(true);
  
  rdebugIln("[OK] Wifi");
  rdebugIln("[OK] RemoteDebug");
  
  ota_setup();
  rdebugIln("[OK] OTA");
  //Serial.println("OTA");delay(10);

  mqtt_setup();
  rdebugIln("[OK] Mqtt");
  //Serial.println("mqtt");delay(10);

  httpServer_setup();
  rdebugIln("[OK] HTTP Server");
  //Serial.println("HTTP");delay(10);

  //alexa_setup();
  //rdebugIln("[OK] Alexa");
  //Serial.println("Alexa");delay(10);

  HTTP.begin();
  rdebugIln("[OK] HTTP Server started");
  //Serial.println("HTTP2");delay(10);

  ntp_setup();
  rdebugIln("[OK] NTP");
  //Serial.println("NTP");delay(10);
  
  elec_setup();
  rdebugIln("[OK] Elec");
  //Serial.println("Elec");delay(10);
  
  ch_setup();
  rdebugIln("[OK] Chauffeau");
  //Serial.println("Chauffeau");delay(10);
  
  rdebugIln("[OK] mDNS : %s.local", _host);
  rdebugIln("[OK] IP   : %s", WiFi.localIP().toString().c_str());

  rdebugIln("*** Setup done ***");
}

void loop() {
  
    if (Debug.isActive(Debug.DEBUG) && (millis() - ntp_t > 15000)) {
        rdebugDln(
            "%s ; uptime %s ; %s (EDF) %s (HOME) ; chauffeau (%s) %s",
            timeClient.getFormattedTime().c_str(),
            ntp_uptime().c_str(),
            isHPEDF() ? "HP" : "HC",
            isHP() ? "HP" : "HC",
            ch_state.c_str(),
            ch_relay_on ? "ON" : "OFF"
        );
        ntp_t = millis();
    }

  elec_handle();
  ch_handle();

  ArduinoOTA.handle();
  Debug.handle();
  mqtt_handle();
  //alexa_handle();
  httpServer_handle();
  ntp_handle();

  delay(10);
}

