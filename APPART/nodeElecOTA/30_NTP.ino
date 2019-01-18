#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

WiFiUDP ntpUDP;
unsigned long startTime = 0;

// By default 'pool.ntp.org' is used with 60 seconds update interval and
// no offset
//NTPClient timeClient(ntpUDP);

// You can specify the time server pool and the offset, (in seconds)
// additionaly you can specify the update interval (in milliseconds).
NTPClient timeClient(
    ntpUDP, 
    "europe.pool.ntp.org",
    3600,   // offset: GMT+1
    300*1000 // update interval 5min
);

void ntp_setup(){
    timeClient.begin();
}

void ntp_handle() {
    timeClient.update();
    if (startTime == 0) startTime = timeClient.getEpochTime();
}


String ntp_uptime() {
    unsigned long rawTime = timeClient.getEpochTime() - startTime;

    unsigned long hours = (rawTime % 86400L) / 3600;
    String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);

    unsigned long days = hours / 24;
    String daysStr = String(days);

    hours = hours - (24 * days);

    unsigned long minutes = (rawTime % 3600) / 60;
    String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

    return daysStr + "d " + hoursStr + "h " + minuteStr + "m";
}

