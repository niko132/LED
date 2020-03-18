#include "FirmwareUpdate.h"

#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>

#include "PhysicalDevice.h"
#include "DeviceManager.h"

DeviceManager *deviceManager = NULL;

AsyncWebServer server(80);

IPAddress ip(192, 168, 178, 113);

IPAddress gateway(192, 168, 178, 1);
IPAddress subnet(255, 255, 255, 0);

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  Serial.println("Booting...");
  
  setupUpdateInterrupt();

  // TODO: setup wifi
  WiFi.config(ip, gateway, subnet);
  WiFi.begin("WGLan", "94384322823429699220");

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

  deviceManager = new DeviceManager(180);

  deviceManager->begin(&server);
  
  server.on("/free_heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Free Heap: " + String(ESP.getFreeHeap()));
  });
  
  server.begin();
}

unsigned long lastAliveMillis = 0;
unsigned long logMillis = 0;

void loop() {
//  Serial.println(String(millis() - logMillis) + "ms");
  logMillis = millis();
	
	
  // put your main code here, to run repeatedly:
  ArduinoOTA.handle();
  
  // TODO: implement heap check
  // Serial.println("Free Heap: " + String(ESP.getFreeHeap()));

  deviceManager->update();
  
  if (millis() - lastAliveMillis > 5 * 1000) {
	  Serial.println("Alive");
	  lastAliveMillis = millis();
  }
}
