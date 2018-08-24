#include <constants.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <functional>

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWD;
String device_name = "test relay";

byte relON[] = {0xA0, 0x01, 0x01, 0xA2};
byte relOFF[] = {0xA0, 0x01, 0x00, 0xA1};


void prepareIds();
boolean connectWifi();
boolean connectUDP();
void startHttpServer();
void respondToSearch();
void turnOnRelay();
void turnOffRelay();


WiFiUDP UDP;
boolean udpConnected = false;
IPAddress ipMulti(239, 255, 255, 250);
unsigned int portMulti = 1900;

ESP8266WebServer HTTP(80);

boolean wifiConnected = false;

char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,

String serial;
String persistent_uuid;
String switch1;



boolean cannotConnectToWifi = false;

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("Start");

  // Setup Relay
  turnOffRelay();

  prepareIds();

  // Initialise wifi connection
  wifiConnected = connectWifi();

  // only proceed if wifi connection successful
  if(wifiConnected){
    udpConnected = connectUDP();
      Serial.println(WiFi.localIP());
      Serial.println(WiFi.macAddress());
    if (udpConnected) {
      // initialise pins if needed
      startHttpServer();
    }
  }
}

void loop() {

  HTTP.handleClient();
  delay(1);


  // if there's data available, read a packet
  // check if the WiFi and UDP connections were successful
  if (wifiConnected && udpConnected) {
    // if there’s data available, read a packet
    int packetSize = UDP.parsePacket();

    if (packetSize) {
      IPAddress remote = UDP.remoteIP();

      int len = UDP.read(packetBuffer, 255);

      if (len > 0) {
          packetBuffer[len] = 0;
      }

      String request = packetBuffer;

      if (request.indexOf('M-SEARCH') >= 0) {
        if (request.indexOf("urn:Belkin:device:**") >= 0) {
          respondToSearch();
        }
      }
    }
    delay(10);
  }
}

void prepareIds() {
  uint32_t chipId = ESP.getChipId();
  char uuid[64];
  sprintf_P(uuid, PSTR("38323636-4558-4dda-9188-cda0e6%02x%02x%02x"),
        (uint16_t) ((chipId >> 16) & 0xff),
        (uint16_t) ((chipId >>  8) & 0xff),
        (uint16_t)   chipId        & 0xff);

  serial = String(uuid);
  persistent_uuid = "Socket-1_0-" + serial;
  //device_name = "box";
}

void respondToSearch() {
    IPAddress localIP = WiFi.localIP();
    char s[16];
    sprintf(s, "%d.%d.%d.%d", localIP[0], localIP[1], localIP[2], localIP[3]);

    String response =
         "HTTP/1.1 200 OK\r\n"
         "CACHE-CONTROL: max-age=86400\r\n"
         "DATE: Fri, 15 Apr 2016 04:56:29 GMT\r\n"
         "EXT:\r\n"
         "LOCATION: http://" + String(s) + ":80/setup.xml\r\n"
         "OPT: \"http://schemas.upnp.org/upnp/1/0/\"; ns=01\r\n"
         "01-NLS: b9200ebb-736d-4b93-bf03-835149d13983\r\n"
         "SERVER: Unspecified, UPnP/1.0, Unspecified\r\n"
         "ST: urn:Belkin:device:**\r\n"
         "USN: uuid:" + persistent_uuid + "::urn:Belkin:device:**\r\n"
         "X-User-Agent: redsonic\r\n\r\n";

    UDP.beginPacket(UDP.remoteIP(), UDP.remotePort());
    UDP.write(response.c_str());
    UDP.endPacket();

}

void startHttpServer() {
    HTTP.on("/", HTTP_GET, [](){
      HTTP.send(200, "text/plain", "OK");
    });

    HTTP.on("/upnp/control/basicevent1", HTTP_POST, []() {

      String request = HTTP.arg(0);

      if(request.indexOf("<BinaryState>1</BinaryState>") >= 0) {
          turnOnRelay();
      }

      if(request.indexOf("<BinaryState>0</BinaryState>") >= 0) {
          turnOffRelay();
      }

      HTTP.send(200, "text/plain", "");
    });


    HTTP.on("/switch", HTTP_GET, []() {
      int request = HTTP.arg(0).toInt();

      if(request == 1) {
          turnOnRelay();
          switch1 = "On";
      }

      if(request == 0) {
          turnOffRelay();
          switch1 = "Off";
      }

      HTTP.send(200, "text/plain", "Switch is " + switch1);
    });

    HTTP.on("/eventservice.xml", HTTP_GET, [](){
      String eventservice_xml = "<?scpd xmlns=\"urn:Belkin:service-1-0\"?>"
            "<actionList>"
              "<action>"
                "<name>SetBinaryState</name>"
                "<argumentList>"
                  "<argument>"
                    "<retval/>"
                    "<name>BinaryState</name>"
                    "<relatedStateVariable>BinaryState</relatedStateVariable>"
                    "<direction>in</direction>"
                  "</argument>"
                "</argumentList>"
                 "<serviceStateTable>"
                  "<stateVariable sendEvents=\"yes\">"
                    "<name>BinaryState</name>"
                    "<dataType>Boolean</dataType>"
                    "<defaultValue>0</defaultValue>"
                  "</stateVariable>"
                  "<stateVariable sendEvents=\"yes\">"
                    "<name>level</name>"
                    "<dataType>string</dataType>"
                    "<defaultValue>0</defaultValue>"
                  "</stateVariable>"
                "</serviceStateTable>"
              "</action>"
            "</scpd>\r\n"
            "\r\n";

      HTTP.send(200, "text/plain", eventservice_xml.c_str());
    });

    HTTP.on("/setup.xml", HTTP_GET, [](){

      IPAddress localIP = WiFi.localIP();
      char s[16];
      sprintf(s, "%d.%d.%d.%d", localIP[0], localIP[1], localIP[2], localIP[3]);

      String setup_xml = "<?xml version=\"1.0\"?>"
            "<root>"
             "<device>"
                "<deviceType>urn:Belkin:device:controllee:1</deviceType>"
                "<friendlyName>"+ device_name +"</friendlyName>"
                "<manufacturer>Belkin International Inc.</manufacturer>"
                "<modelName>Emulated Socket</modelName>"
                "<modelNumber>3.1415</modelNumber>"
                "<UDN>uuid:"+ persistent_uuid +"</UDN>"
                "<serialNumber>221517K0101769</serialNumber>"
                "<binaryState>0</binaryState>"
                "<serviceList>"
                  "<service>"
                      "<serviceType>urn:Belkin:service:basicevent:1</serviceType>"
                      "<serviceId>urn:Belkin:serviceId:basicevent1</serviceId>"
                      "<controlURL>/upnp/control/basicevent1</controlURL>"
                      "<eventSubURL>/upnp/event/basicevent1</eventSubURL>"
                      "<SCPDURL>/eventservice.xml</SCPDURL>"
                  "</service>"
              "</serviceList>"
              "</device>"
            "</root>\r\n"
            "\r\n";

        HTTP.send(200, "text/xml", setup_xml.c_str());
    });

    HTTP.begin();
    Serial.println("HTTP Server started...");
}


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

boolean connectUDP(){
  boolean state = false;

  if(UDP.beginMulticast(WiFi.localIP(), ipMulti, portMulti)) {
    state = true;
  }

  return state;
}

void turnOnRelay() {
 Serial.write(relON, sizeof(relON));
}

void turnOffRelay() {
  Serial.write(relOFF, sizeof(relOFF));
}
