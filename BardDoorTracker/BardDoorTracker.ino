/**
 * https://randomnerdtutorials.com/esp32-bluetooth-low-energy-ble-arduino-ide/
 * https://platformio.org/lib/show/1841/ESP32%20BLE%20Arduino/examples?file=BLE_uart.ino
 * https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/BLECharacteristic.h
 *
 * https://www.uuidgenerator.net/version4
 */

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// https://www.uuidgenerator.net/
#define SERVICE_UUID        "30a3bf65-2a1b-49b2-8d70-6179f354960d"
#define C_POWER_UUID        "a1883e01-41c4-4d0c-85f4-63c49ad1121e"
#define C_DIRECTION_UUID    "9b1262e8-34aa-474f-85a4-e7b761fecb11"
#define C_SPEED_UUID        "61b9a2c4-31af-4a0b-a8b9-5d50b80246c7"
#define C_SPEED_VAL_UUID    "ebb1ea2b-443b-4f7e-ae44-a0ff4195fb29"

#define C_PROPERTIES       BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE



#include <EEPROM.h>
#define EEPROM_SIZE 10


const int LED_BUILTIN = 2;

const int PIN_MOTOR_1 = 33;
const int PIN_MOTOR_2 = 32;
const int PIN_MOTOR_3 = 35;
const int PIN_MOTOR_4 = 34;

const int UP    = LOW;
const int DOWN  = HIGH;

const int FAST  = LOW;
const int SLOW  = HIGH;

const int ON    = HIGH;
const int OFF   = LOW;

const byte patterns[8] = {
  0b00001000,
  0b00001100,
  0b00000100,
  0b00000110,
  0b00000010,
  0b00000011,
  0b00000001,
  0b00001001,
};

unsigned long t1 = 0, t2 = 0;

bool button_power = OFF;
bool button_dir = DOWN;
bool button_speed = SLOW;
long speed_value = 9317;

const int adr_power     = 1;
const int adr_direction = 2;
const int adr_speed     = 3;
const int adr_speed_val = 4;



class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      Serial.println("Device connected");
    };

    void onDisconnect(BLEServer* pServer) {
      Serial.println("Device disconnected");
    }
};

class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
  private:
  const char * m_uuid;
  public:
  MyCharacteristicCallbacks(const char* uuid) {
    m_uuid = uuid;
  }
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
      Serial.print("New value: ");
      Serial.println(m_uuid);
      Serial.print(" = ");
      Serial.println(value.c_str());
      if (m_uuid == C_POWER_UUID) {
          button_power = value == "on" ? ON : OFF;
          EEPROM.put(adr_power, button_power);
          EEPROM.commit();
          digitalWrite(LED_BUILTIN, button_power ? LOW : HIGH);
      }
      else if (m_uuid == C_DIRECTION_UUID) {
          button_dir = value == "up" ? UP : DOWN;
          EEPROM.put(adr_direction, button_dir);
          EEPROM.commit();
      }
      else if (m_uuid == C_SPEED_UUID) {
          button_speed = value == "slow" ? SLOW : FAST;
          EEPROM.put(adr_speed, button_speed);
          EEPROM.commit();
      }
      else if (m_uuid == C_SPEED_VAL_UUID) {
          speed_value = String(value.c_str()).toInt(); // Erk...
          EEPROM.put(adr_speed_val, speed_value);
          EEPROM.commit();
      }
    }
  }
};

void setup() {

  Serial.begin(74880);
  while (!Serial);
  Serial.println("OK");

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_MOTOR_1, OUTPUT);
  pinMode(PIN_MOTOR_2, OUTPUT);
  pinMode(PIN_MOTOR_3, OUTPUT);
  pinMode(PIN_MOTOR_4, OUTPUT);

  EEPROM.begin(EEPROM_SIZE);
  
  /* INIT EEPROM 
  EEPROM.put(adr_power, OFF);
  EEPROM.put(adr_direction, DOWN);
  EEPROM.put(adr_speed, SLOW);
  EEPROM.put(adr_speed_val, 9000);
  EEPROM.commit();
  /**/

  EEPROM.get(adr_power, button_power);
  EEPROM.get(adr_direction, button_dir);
  EEPROM.get(adr_speed, button_speed);
  EEPROM.get(adr_speed_val, speed_value);
  
  digitalWrite(LED_BUILTIN, button_power ? LOW : HIGH);
  
  Serial.println("Starting BLE...");
  setup_bluetooth();
  
  Serial.println("Done");
}


void setup_bluetooth() {
  // Create the BLE Device
  BLEDevice::init("ESP32");

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  BLECharacteristic *pCharacteristic;
  pCharacteristic = pService->createCharacteristic(C_POWER_UUID, C_PROPERTIES);
  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks(C_POWER_UUID));
  pCharacteristic->setValue(button_power == OFF ? "off" : "on"); // Erk...

  pCharacteristic = pService->createCharacteristic(C_DIRECTION_UUID, C_PROPERTIES);
  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks(C_DIRECTION_UUID));
  pCharacteristic->setValue(button_dir == DOWN ? "down" : "up"); // Erk...

  pCharacteristic = pService->createCharacteristic(C_SPEED_UUID, C_PROPERTIES);
  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks(C_SPEED_UUID));
  pCharacteristic->setValue(button_speed == SLOW ? "slow" : "fast"); // Erk...

  pCharacteristic = pService->createCharacteristic(C_SPEED_VAL_UUID, C_PROPERTIES);
  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks(C_SPEED_VAL_UUID));
  pCharacteristic->setValue(String(speed_value).c_str()); // Erk...

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  
  // ???
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  
  BLEDevice::startAdvertising();
}


void loop() {
  //t2 = micros();
  //Serial.println(t2-t1);
  //delay(10);
  //t1 = micros();
  // Tps mesuré : 9592us

  if (button_power == ON) {
    if (button_speed == SLOW) {
      step();
      delayMicroseconds(speed_value);
    }
    else {
      step2();
      delay(2);    
    }
  }
}


/*
 * https://forum.arduino.cc/index.php?topic=574391.msg3911823#msg3911823
 * 
 */
void step() {
  static int i = 0;
  digitalWrite(PIN_MOTOR_1, bitRead(patterns[i], button_dir == UP ? 0 : 3));
  digitalWrite(PIN_MOTOR_2, bitRead(patterns[i], button_dir == UP ? 1 : 2));
  digitalWrite(PIN_MOTOR_3, bitRead(patterns[i], button_dir == UP ? 2 : 1));
  digitalWrite(PIN_MOTOR_4, bitRead(patterns[i], button_dir == UP ? 3 : 0));
  i=++i%8;
}

void step2() {
  static int j = 0;
  digitalWrite(PIN_MOTOR_1, bitRead(patterns[j], button_dir == UP ? 0 : 3));
  digitalWrite(PIN_MOTOR_2, bitRead(patterns[j], button_dir == UP ? 1 : 2));
  digitalWrite(PIN_MOTOR_3, bitRead(patterns[j], button_dir == UP ? 2 : 1));
  digitalWrite(PIN_MOTOR_4, bitRead(patterns[j], button_dir == UP ? 3 : 0));
  j=(j+2)%8;
}
