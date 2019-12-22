#include <Arduino.h>

#include "PhysicalDevice.h"
#include "DeviceManager.h"

DeviceManager deviceManager(180);

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);

  // TODO: setup wifi

  deviceManager.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
}