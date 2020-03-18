#ifndef FIRMWAREUPDATE_H
#define FIRMWAREUPDATE_H

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>
// #include <FS.h>

ICACHE_RAM_ATTR void triggerOTA() {
	Serial.println("Here I am!");

	EEPROM.write(0, 1); // write a flag to trigger update mode
	EEPROM.commit();

	ESP.restart();
}

void setupUpdateInterrupt()
{
	EEPROM.begin(4);

	pinMode(14, INPUT);
	attachInterrupt(digitalPinToInterrupt(14), triggerOTA, CHANGE);
}

void checkUpdate()
{
	if (EEPROM.read(0) != 0) {
		EEPROM.write(0, 0);
		EEPROM.commit();
    
		// do other stuff
		Serial.println("PREPARING UPDATE...");

		while (true) {
			yield();
			ArduinoOTA.handle();
		}
	}
}


void setupOTA()
{
	ArduinoOTA.onStart([]() {
		String type;
		if (ArduinoOTA.getCommand() == U_FLASH) {
			type = "sketch";
		} else { // U_SPIFFS
			type = "filesystem";
		}

		// NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
		SPIFFS.end();
    
		Serial.println("Start updating " + type);
	});
 
	ArduinoOTA.onEnd([]() {
		Serial.println("End");
	});
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
		Serial.printf("Progress: %u\n", (progress / (total / 100)));
	});
	ArduinoOTA.onError([](ota_error_t error) {
		Serial.printf("Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) {
			Serial.println("Auth Failed");
		} else if (error == OTA_BEGIN_ERROR) {
			Serial.println("Begin Failed");
		} else if (error == OTA_CONNECT_ERROR) {
			Serial.println("Connect Failed");
		} else if (error == OTA_RECEIVE_ERROR) {
			Serial.println("Receive Failed");
		} else if (error == OTA_END_ERROR) {
			Serial.println("End Failed");
		}
	});
	ArduinoOTA.begin();
	
	checkUpdate();
}

#endif // FIRMWAREUPDATE_H
