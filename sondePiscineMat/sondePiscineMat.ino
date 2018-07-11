// https://create.arduino.cc/projecthub/TheGadgetBoy/ds18b20-digital-temperature-sensor-and-arduino-9cc806
// https://letmeknow.fr/blog/2016/08/10/le-capteur-de-temperature-ds18b20/

#include <OneWire.h> //Librairie du bus OneWire
#include <DallasTemperature.h> //Librairie du capteur

OneWire oneWire(2); //Bus One Wire sur la pin 2 de l'arduino
DallasTemperature sensors(&oneWire); //Utilistion du bus Onewire pour les capteurs
DeviceAddress sensorDeviceAddress; //Vérifie la compatibilité des capteurs avec la librairie

void setup(void){
 Serial.begin(74880); //Permet la communication en serial
 pinMode(2, INPUT);           // set pin to input
 digitalWrite(2, HIGH);       // turn on pullup resistors

 sensors.begin(); //Activation des capteurs
 sensors.getAddress(sensorDeviceAddress, 0); //Demande l'adresse du capteur à l'index 0 du bus
 sensors.setResolution(sensorDeviceAddress, 10); //Résolutions possibles: 9,10,11,12
}

void loop(void){
 sensors.requestTemperatures(); //Demande la température aux capteurs
 Serial.print(sensors.getTempCByIndex(0)); //Récupération de la température en celsius du capteur n°0
 Serial.println(" °C");
 delay(1000);
}
