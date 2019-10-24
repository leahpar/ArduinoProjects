/*
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleServer.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updates by chegewara
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>


const int LED_BUILTIN = 2;

// https://www.uuidgenerator.net/
#define SERVICE_UUID        "30a3bf65-2a1b-49b2-8d70-6179f354960d"
#define C_POWER_UUID        "a1883e01-41c4-4d0c-85f4-63c49ad1121e"
#define C_DIRECTION_UUID    "9b1262e8-34aa-474f-85a4-e7b761fecb11"
#define C_SPEED_UUID        "ebb1ea2b-443b-4f7e-ae44-a0ff4195fb29"

#define C_PROPERTIES       BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE

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
      if (value == "off") {
        digitalWrite(LED_BUILTIN, HIGH);
      } else {
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
  }
};


void setup() {
  Serial.begin(74880);
  while (!Serial);
  Serial.println();

  pinMode(LED_BUILTIN, OUTPUT);

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
  pCharacteristic->setValue("off");

  pCharacteristic = pService->createCharacteristic(C_DIRECTION_UUID, C_PROPERTIES);
  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks(C_DIRECTION_UUID));
  pCharacteristic->setValue("down");

  pCharacteristic = pService->createCharacteristic(C_SPEED_UUID, C_PROPERTIES);
  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks(C_SPEED_UUID));
  pCharacteristic->setValue("9000");

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
  // put your main code here, to run repeatedly:
  delay(2000);
}
