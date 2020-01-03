#include <Arduino.h>

#include "PhysicalDevice.h"
#include "DeviceManager.h"

DeviceManager *deviceManager = NULL;

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);

  Serial.println("Booting...");

  // TODO: setup wifi

  deviceManager = new DeviceManager(30);

  deviceManager->begin();
  deviceManager->addDevice(10, 20, 1, 0);
  deviceManager->addDevice(8, 12, 1, 0);
  int id = deviceManager->addDevice(11, 13, 2, 0);
  deviceManager->debug();

  deviceManager->editDevice(id, 22, 28, 2, 0);
  deviceManager->debug();

  Serial.println("Test");
}

void loop() {
  // put your main code here, to run repeatedly:

  deviceManager->update();
}