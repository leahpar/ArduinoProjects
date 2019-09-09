// Doc Sigfox
// https://forum.arduino.cc/index.php?topic=478766.0

// --- GPS --------------------------------------------------------------------

// http://arduiniana.org/libraries/tinygpsplus/

// GPS tags : http://aprs.gids.nl/nmea/

#include <TinyGPS++.h>
TinyGPSPlus gps;

// SoftwareSerial pour l'arduino nano
#include <SoftwareSerial.h>
SoftwareSerial Serial1(/*RX*/ 3, /*TX*/ 2);

char gps_data;

// --- SD Card -----------------------------------------------------------------

#include <SPI.h>
#include <SD.h>

// MOSI <=> D11
// MISO <=> D12
// SCK  <=> D13
const int PIN_SD_CS = 4;

File myFile;

// --- OLED --------------------------------------------------------------------

// much lighter than adafruid library
// https://github.com/greiman/SSD1306Ascii

#include "SSD1306Ascii.h"
#include "SSD1306AsciiAvrI2c.h"


// SCL <=> A5
// SDA <=> A4
// VCC <=> 3.3V ~ 5V
// GND <=> GND

#define I2C_ADDRESS 0x3C
SSD1306AsciiAvrI2c oled;

// --- Others --------------------------------------------------------------------

const unsigned long interval = 10000; // ms
unsigned long writeTimer = 0;
unsigned long gpsTimer = 0;
char buffer[100];

void setup() {
    Serial.begin(74880);
    while (!Serial);
    Serial.println(F("Setup..."));
    
    // Init GPS
    Serial.print(F("GPS..."));
    Serial1.begin(9600);
    Serial.println(F("Done"));
    delay(50);
    
    // Init OLED
    Serial.print(F("OLED..."));
    oled.begin(&Adafruit128x64, I2C_ADDRESS);
    oled.setFont(Adafruit5x7);
    oled.clear();
    Serial.println(F("Done"));
    delay(50);
    
    // Init SD
    /*
    Serial.print(F("SD..."));
    pinMode(PIN_SD_CS, OUTPUT);
    digitalWrite(PIN_SD_CS, HIGH);
    if (!SD.begin(PIN_SD_CS)) {
        Serial.println(F("failed!"));
        oled.println(F("SD initialization failed!"));
        while (1);
    }
    Serial.println(F("Done"));
    delay(50);
    */

    Serial.println(F("Setup done"));
}

int i = 0;
void loop() {

    // Read data from GPS receiver
    while (Serial1.available() > 0) {
        gps_data = Serial1.read();
        if (!gps.location.isUpdated()) {
            // No location, dump raw data
            Serial.print(gps_data);
            if (millis() - gpsTimer > 5000) {
                gpsTimer = millis();
                oled.clear();
                oled.println(F("GPS..."));
                Serial.println();
            }
        }
        gps.encode(gps_data);
    }

    if (gps.location.isUpdated()) {

        // Screen
        if (millis() - gpsTimer > 1000) {
            gpsTimer = millis();
            oled_display();
        }

        // File
        /*
        if (millis() - writeTimer > interval) {
            myFile = SD.open("gps.txt", FILE_WRITE);
            if (myFile) {
              wrideGpsData();
              myFile.close();
            }
            else {
              Serial.println(F("error openning file"));
              oled.clear();
              oled.println(F("error openning file"));
            }
        }
        */
    }
    
    delay(10);
}

void wrideGpsData() {
    sprintf(
        buffer,
        "%d-%d-%d %d:%d:%d;%s;%s;%s;%d;%d",
        gps.date.year(),
        gps.date.month(),
        gps.date.day(),
        gps.time.hour()+2,
        gps.time.minute(),
        gps.time.second(),
        String(gps.location.lat(), 7).c_str(),
        String(gps.location.lng(), 7).c_str(),
        String(gps.speed.mps(), 2).c_str(),
        gps.satellites.value(),
        gps.hdop.value()
    );
    Serial.println(buffer);
    myFile.println(buffer);
}


void oled_display() {
    oled.setCursor(0, 0);
    oled.set2X();
    
    sprintf(
        buffer,
        "%d:%d:%d",
        gps.time.hour()+2,
        gps.time.minute(),
        gps.time.second()
    );
    oled.println(buffer);
    
    sprintf(
        buffer,
        "%s %s",
        String(gps.location.lat(), 2).c_str(),
        String(gps.location.lng(), 2).c_str()
    );
    oled.println(buffer);

    sprintf(
        buffer,
        "%d %d",
        gps.satellites.value(),
        gps.hdop.value()
    );
    oled.println(buffer);

    sprintf(
        buffer,
        "%s km/h",
        String(gps.speed.kmph(), 0).c_str()
    );
    oled.println(buffer);
}

