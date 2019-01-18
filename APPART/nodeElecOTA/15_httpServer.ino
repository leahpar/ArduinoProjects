#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

ESP8266WebServer HTTP(80);


void httpServer_setup()
{
    HTTP.on("/", HTTP_GET, [](){
      HTTP.send(200, "text/plain", "OK");
    });
}



void httpServer_handle()
{
    HTTP.handleClient();
  
}