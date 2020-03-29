#include "FirmwareUpdate.h"

#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>

#include "PhysicalDevice.h"
#include "DebugDevice.h"

#include "DeviceManager.h"
#include "SyncManager.h"

AsyncWebServer server(80);

/*
IPAddress ip(192, 168, 178, 113);

IPAddress gateway(192, 168, 178, 1);
IPAddress subnet(255, 255, 255, 0);
*/

IPAddress ip(192, 168, 2, 113);

IPAddress gateway(192, 168, 2, 1);
IPAddress subnet(255, 255, 255, 0);

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  Serial.println("Booting...");
  
  setupUpdateInterrupt();

  // TODO: setup wifi
  WiFi.config(ip, gateway, subnet);
  // WiFi.begin("WGLan", "94384322823429699220");
  WiFi.begin("WLAN-RN22NX", "5152111417820959");

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  setupOTA();
  
  // Initialize SPIFFS
  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // deviceManager = new DeviceManager(180);
  DebugDevice *dd = new DebugDevice(180);
  LEDDeviceManager.begin(dd, &server);
  LEDSyncManager.begin(&server);
  
  server.on("/free_heap", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Free Heap: " + String(ESP.getFreeHeap()));
  });
  
  server.on("/heap_frag", HTTP_GET, [](AsyncWebServerRequest *request) {
	  float frag = ESP.getHeapFragmentation();
	  
	  request->send(200, "text/plain", "Heap Fragmentation: " + String(frag) + "%");
  });
  
  server.begin();
}

unsigned long lastAliveMillis = 0;

const int FPS_INTERVAL = 5;
unsigned long fpsNextMillis = 0;
unsigned long frameCounter = 0;

void loop() {
  // Print FPS
  /*
  frameCounter++;
  if (millis() > fpsNextMillis) {
	  int fps = (float) frameCounter / FPS_INTERVAL;
	  Serial.println(String(fps) + "FPS");
	  
	  fpsNextMillis = millis() + FPS_INTERVAL * 1000;
	  frameCounter = 0;
  }
  */
	
	
  // put your main code here, to run repeatedly:
  ArduinoOTA.handle();
  
  // TODO: implement heap check
  // Serial.println("Free Heap: " + String(ESP.getFreeHeap()));

  LEDDeviceManager.update();
  LEDSyncManager.update();
  
  if (millis() - lastAliveMillis > 5 * 1000) {
	  Serial.println("Alive");
	  lastAliveMillis = millis();
  }
}
