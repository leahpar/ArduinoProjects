/*
  SendCodeOnOff.h - Example of the library for controlling DiO devices.
  Created by Charles GRASSIN, July 31, 2017.
  Under MIT License (free to use, modify, redistribute, etc.).
  www.charleslabs.fr

  This exemple blinks the DiO device with a period of 4 second.
  Prior to using it, you *must* set the "on" and "off" values using
  the RecordCode example. These values are indeed unique to each 
  module.

  Pin 2 must be connected to a 433MHz Tx module.
*/

#include <DiOremote.h>

const unsigned long CODES[]  = {
1404155792 , 1404155776,
1404155793 , 1404155777,
1404155794 , 1404155778,
1404155795 , 1404155779
};
int i = 0;

const int TX_PIN = 2;

DiOremote myRemote = DiOremote(TX_PIN);

void setup()
{
  Serial.begin(74880);
}

void loop() {
  
  myRemote.send(CODES[i++]);
  delay(1000);
  i = i%8;
}
