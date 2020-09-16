#include "DeviceManager.h"

#include "EffectManager.h"

#include "ColorCycle.h"
#include "ColorFade.h"
#include "StaticColor.h"
#include "Snake.h"
#include "PingPong.h"
#include "CustomEffect.h"

#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <FS.h>

void DeviceManager::begin(DebugDevice *device, AsyncWebServer *server)
{
	_device = device;
	
	bool success = loadConfig(device->getLedCount());
	
	if (!success) {
		int ledCount = device->getLedCount();
		addDevice(0, ledCount, 0, 0);
	}
	
	begin(server);
}

void DeviceManager::begin(unsigned int ledCount, AsyncWebServer *server)
{
	loadConfig(ledCount);
	
	if (!_device) { // couldnt load proper config
		_device = new PhysicalDevice(ledCount);
		addDevice(0, ledCount, 0, 0);
	}
	
	begin(server);
}

DeviceManager::~DeviceManager()
{
    if (_device) {
        delete _device;
        _device = NULL;
    }

    // TODO: delete deviceHierarchy
}

void DeviceManager::begin(AsyncWebServer *server)
{
	_server = server;
	
    if (_device)
        _device->begin();

	for (int row = 0; row < _deviceHierarchy.size(); row++) {
		for (int col = 0; col < _deviceHierarchy[row]->size(); col++) {
			_deviceHierarchy[row]->at(col)->begin(server);
		}
	}
	
	AsyncCallbackJsonWebHandler *addHandler = new AsyncCallbackJsonWebHandler("/add_device", [this](AsyncWebServerRequest *request, JsonVariant &json) {
		JsonObject jsonObj = json.as<JsonObject>();
		
		int startIndex = -1; // mandatory
		int endIndex = -1; // mandatory
		int zIndex = 0; // default value
		int mode = 0; // default value
		
		if (jsonObj.containsKey("startIndex") && jsonObj.containsKey("endIndex")) {
			startIndex = jsonObj["startIndex"].as<int>();
			endIndex = jsonObj["endIndex"].as<int>();
			
			if (jsonObj.containsKey("zIndex"))
				zIndex = jsonObj["zIndex"].as<int>();
			
			if (jsonObj.containsKey("mode"))
				mode = jsonObj["mode"].as<int>();
			
			Serial.println("Add: " + String(startIndex) + " " + String(endIndex) + " " + String(zIndex) + " " + String(mode));
	
			unsigned long id = addDevice(startIndex, endIndex, zIndex, mode);
			
			
			saveConfig();
			
			
			request->send(200, "text/plain", String(id));
		} else {
			request->send(400, "text/plain", "startIndex, endIndex needed");
		}
	});
	
	AsyncCallbackJsonWebHandler *editHandler = new AsyncCallbackJsonWebHandler("/edit_device", [this](AsyncWebServerRequest *request, JsonVariant &json) {
		JsonObject jsonObj = json.as<JsonObject>();
		
		unsigned long id = 0; // mandatory
		int startIndex = -1;
		int endIndex = -1;
		int zIndex = -1;
		int mode = -1;
		
		if (jsonObj.containsKey("id")) {
			id = jsonObj["id"].as<unsigned long>();
			
			if (jsonObj.containsKey("startIndex")) {
				startIndex = jsonObj["startIndex"].as<int>();
			}
			
			if (jsonObj.containsKey("endIndex")) {
				endIndex = jsonObj["endIndex"].as<int>();
			}
			
			if (jsonObj.containsKey("zIndex")) {
				zIndex = jsonObj["zIndex"].as<int>();
			}
			
			if (jsonObj.containsKey("mode")) {
				mode = jsonObj["mode"].as<int>();
			}
			
			Serial.println("Edit: " + String(id) + " " + String(startIndex) + " " + String(endIndex) + " " + String(zIndex) + " " + String(mode));
	
			editDevice(id, startIndex, endIndex, zIndex, mode);
			
			
			request->send(200, "text/plain", String(id));
		} else {
			request->send(400, "text/plain", "id needed");
		}
	});
	
	AsyncCallbackJsonWebHandler *removeHandler = new AsyncCallbackJsonWebHandler("/remove_device", [this](AsyncWebServerRequest *request, JsonVariant &json) {
		JsonObject jsonObj = json.as<JsonObject>();
		
		unsigned long id = 0; // mandatory
		
		if (jsonObj.containsKey("id")) {
			id = jsonObj["id"].as<unsigned long>();
			
			Serial.println("Remove: " + String(id));
	
			VirtualDevice* device = removeDevice(id);
			if (device) {
				delete device;
				device = NULL;
			}
			
			
			saveConfig();
			
			
			request->send(200);
		} else {
			request->send(400, "text/plain", "id needed");
		}
	});
	
	AsyncCallbackJsonWebHandler *onOffHandler = new AsyncCallbackJsonWebHandler("/set_on_off", [this](AsyncWebServerRequest *request, JsonVariant &json) {
		JsonObject jsonObj = json.as<JsonObject>();
		
		bool on = false; // mandatory
		
		if (jsonObj.containsKey("on")) {
			on = jsonObj["on"].as<bool>();
			
			Serial.println("Set On Off: " + String(on));
		
			_onState = on;
			
			
			saveConfig();
			
			
			request->send(200);
		} else {
			request->send(400, "text/plain", "on needed");
		}
	});
	
	server->addHandler(addHandler);
	server->addHandler(editHandler);
	server->addHandler(removeHandler);
	server->addHandler(onOffHandler);
	
	server->on("/get_info", HTTP_GET, [this](AsyncWebServerRequest *request){
		AsyncJsonResponse *response = new AsyncJsonResponse();
		JsonObject root = response->getRoot().as<JsonObject>();
		
		root["on"] = _onState;
		
		response->setLength();
		request->send(response);
	});
	
	server->on("/get_effects", HTTP_GET, [this](AsyncWebServerRequest *request){
		AsyncJsonResponse *response = new AsyncJsonResponse();
		JsonArray root = response->getRoot().to<JsonArray>();
		
		/*
		Effect* effects[] = {
			new ColorCycle(NULL),
			new ColorFade(NULL),
			new StaticColor(NULL),
			new Snake(NULL),
			new PingPong(NULL)
		};
		
		// TODO: implement variable effect size
		for (int i = 0; i < 5; i++) {
			JsonObject deviceObject = root.createNestedObject();
			deviceObject["index"] = i;
			deviceObject["name"] = effects[i]->getName();
			
			
			// TODO: get some better memory management
			delete effects[i];
			effects[i] = NULL;
		}
		*/
		
		unsigned int effectCount = LEDEffectManager.getEffectCount();
		
		for (int i = 0; i < effectCount; i++) {
			JsonObject deviceObject = root.createNestedObject();
			deviceObject["index"] = i;
			deviceObject["name"] = LEDEffectManager.getEffectNameAt(i);
		}
		
		response->setLength();
		request->send(response);
	});
	
	server->on("/get_devices", HTTP_GET, [this](AsyncWebServerRequest *request){
		AsyncJsonResponse *response = new AsyncJsonResponse(true, 4 * 1024);
		JsonArray root = response->getRoot().as<JsonArray>();
		
		for (int i = 0; i < getDeviceCount(); i++) {
			JsonObject deviceObject = root.createNestedObject();
			deviceObject["startIndex"] = getDeviceAt(i)->getStartIndex();
			deviceObject["endIndex"] = getDeviceAt(i)->getEndIndex();
			deviceObject["ledCount"] = getDeviceAt(i)->getLedCount();
			deviceObject["ledRangeCount"] = getDeviceAt(i)->getLedRangeCount();
			deviceObject["zIndex"] = getDeviceZIndex(getDeviceAt(i));
			deviceObject["mode"] = getDeviceAt(i)->getMode();
			deviceObject["effectIndex"] = getDeviceAt(i)->getEffectIndex();
			deviceObject["posStart"] = getDeviceAt(i)->getPosStart();
			deviceObject["posEnd"] = getDeviceAt(i)->getPosEnd();
			deviceObject["id"] = getDeviceAt(i)->getId();
		}
		
		response->setLength();
		request->send(response);
	});
	
	server->on("/", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(SPIFFS, "/index.html", String());
	});
}

bool DeviceManager::loadConfig(unsigned int ledCount)
{
	Serial.println("Loading Config...");
	
	String filename = "config.cfg";	
	File cfgFile = SPIFFS.open(filename, "r");
	
	if (!cfgFile) {
		Serial.println("Config file doesnt exist");
		return false;
	}
	
	cfgFile.readBytes((char*) &_onState, sizeof(_onState));
	
	unsigned int physicalLedCount;
	cfgFile.readBytes((char*) &physicalLedCount, sizeof(physicalLedCount));
	
	if (physicalLedCount != ledCount) {
		cfgFile.close();
		return false;
	}
	
	if (!_device)
		_device = new PhysicalDevice(physicalLedCount);
	
	unsigned int numRows;
	cfgFile.readBytes((char*) &numRows, sizeof(numRows));
	
	Serial.println("Loading " + String(numRows) + " rows");
	
	for (int row = 0; row < numRows; row++) {
		unsigned int numCols;
		cfgFile.readBytes((char*) &numCols, sizeof(numCols));
		
		Serial.println("Row #" + String(row) + ": " + String(numCols) + " cols");
		
		std::vector<VirtualDevice*>* rowVector = new std::vector<VirtualDevice*>();
		
		for (int col = 0; col < numCols; col++) {
			unsigned long deviceId;
			cfgFile.readBytes((char*) &deviceId, sizeof(deviceId));
			
			VirtualDevice* device = new VirtualDevice(_device, deviceId);
			rowVector->push_back(device);
		}
		
		_deviceHierarchy.push_back(rowVector);
	}
	
	cfgFile.close();
	
	return true;
}

void DeviceManager::saveConfig()
{
	Serial.println("Saving Config...");
	
	String filename = "config.cfg";
	File cfgFile = SPIFFS.open(filename, "w");
	
	cfgFile.write((unsigned char*) &_onState, sizeof(_onState));
	
	unsigned int physicalLedCount = 0;
	if (_device)
		physicalLedCount = _device->getLedCount();
	
	cfgFile.write((unsigned char*) &physicalLedCount, sizeof(physicalLedCount));
	
	unsigned int numRows = _deviceHierarchy.size();
	cfgFile.write((unsigned char*) &numRows, sizeof(numRows));
	
	for (int row = 0; row < numRows; row++) {
		unsigned int numCols = _deviceHierarchy[row]->size();
		cfgFile.write((unsigned char*) &numCols, sizeof(numCols));
		
		for (int col = 0; col < numCols; col++) {
			VirtualDevice* device = _deviceHierarchy[row]->at(col);
			
			// just write the id of every device down
			unsigned long deviceId = device->getId();
			cfgFile.write((unsigned char*) &deviceId, sizeof(deviceId));
			
			// the device should autosave itself -> no need to save it one more time
			/*
			// and the save the device in its own file
			// device->serialize();
			*/
		}
	}
	
	cfgFile.close();
}

VirtualDevice* DeviceManager::getDevice(unsigned long id)
{
	for (int row = 0; row < _deviceHierarchy.size(); row++) {
        for (int col = 0; col < _deviceHierarchy[row]->size(); col++) {
            if (id == _deviceHierarchy[row]->at(col)->getId()) {
                return _deviceHierarchy[row]->at(col);
            }
        }
    }
	
	return NULL;
}

VirtualDevice* DeviceManager::getDeviceAt(int index)
{
	for (int row = 0; row < _deviceHierarchy.size(); row++) {
		if (index < _deviceHierarchy[row]->size())
			return _deviceHierarchy[row]->at(index);
		
		index -= _deviceHierarchy[row]->size();
	}
	
	return NULL;
}

int DeviceManager::getDeviceZIndex(VirtualDevice *device)
{
	for (int row = 0; row < _deviceHierarchy.size(); row++) {
        for (int col = 0; col < _deviceHierarchy[row]->size(); col++) {
            if (device == _deviceHierarchy[row]->at(col)) {
                return row;
            }
        }
    }
	
	return -1;
}

int DeviceManager::getDeviceCount()
{
	int count = 0;
	
	for (int row = 0; row < _deviceHierarchy.size(); row++) {
		count += _deviceHierarchy[row]->size();
	}
	
	return count;
}

unsigned long DeviceManager::addDevice(int startIndex, int endIndex, int zIndex, int mode)
{
    Serial.println("Adding Device1: " + String(startIndex) + " " + String(endIndex));

    VirtualDevice *newDevice = new VirtualDevice(_device, startIndex, endIndex, mode);
	
	if (_server)
		newDevice->begin(_server);
	
    addDevice(newDevice, zIndex);

    return newDevice->getId();
}

void DeviceManager::addDevice(VirtualDevice *newDevice, int zIndex) {
    int startIndex = newDevice->getStartIndex();
    int endIndex = newDevice->getEndIndex();

    Serial.println("Adding Device: " + String(startIndex) + " " + String(endIndex));

    if (zIndex >= _deviceHierarchy.size()) {
        _deviceHierarchy.push_back(new std::vector<VirtualDevice*>());
        zIndex = _deviceHierarchy.size() - 1;
    }

    for (int i = _deviceHierarchy[zIndex]->size() - 1; i >= 0; i--) {
        int start = _deviceHierarchy[zIndex]->at(i)->getStartIndex();
        int end = _deviceHierarchy[zIndex]->at(i)->getEndIndex();

        if (start < startIndex && end >= startIndex) {
            _deviceHierarchy[zIndex]->at(i)->setEndIndex(startIndex);
        } else if (start <= endIndex && end > endIndex) {
            _deviceHierarchy[zIndex]->at(i)->setStartIndex(endIndex);
        } else if (start >= startIndex && end <= endIndex) {
            _deviceHierarchy[zIndex]->erase(_deviceHierarchy[zIndex]->begin() + i);
        }
    }

    for (int i = 0; i < _deviceHierarchy[zIndex]->size(); i++) {
        Serial.println("Hierarchy " + String(i) + ": " + String(_deviceHierarchy[zIndex]->at(i)->getStartIndex()) + " " + String(_deviceHierarchy[zIndex]->at(i)->getEndIndex()));
    }

    bool inserted = false;

    for (int i = 0; i < _deviceHierarchy[zIndex]->size(); i++) {
        Serial.println("For: " + String(i));

        if (startIndex < _deviceHierarchy[zIndex]->at(i)->getStartIndex()) {
            Serial.println("Inserting: " + String(i) + " " + String(newDevice->getId()));

            _deviceHierarchy[zIndex]->insert(_deviceHierarchy[zIndex]->begin() + i, newDevice);
            inserted = true;
            break;
        }
    }

    if (!inserted)
        _deviceHierarchy[zIndex]->push_back(newDevice);

    Serial.println("Size: " + String(_deviceHierarchy.size()) + " " + String(_deviceHierarchy[zIndex]->size()));

    buildDevices();
}

bool DeviceManager::editDevice(int id, int startIndex, int endIndex, int zIndex, int mode)
{
	VirtualDevice* device = getDevice(id);
	return editDevice(device, startIndex, endIndex, zIndex, mode);
}

bool DeviceManager::editDevice(VirtualDevice *device, int startIndex, int endIndex, int zIndex, int mode)
{
	if (!device)
		return false;
	
	// use current values
	if (startIndex == -1)
		startIndex = device->getStartIndex();
	
	if (endIndex == -1)
		endIndex = device->getEndIndex();
	
	if (zIndex == -1)
		zIndex = getDeviceZIndex(device);
	
	if (mode == -1)
		mode = device->getMode();
	
    removeDevice(device);

    device->setStartIndex(startIndex);
    device->setEndIndex(endIndex);
    device->setMode(mode);

    addDevice(device, zIndex);
	
	return true;
}

VirtualDevice* DeviceManager::removeDevice(int id)
{
	VirtualDevice* device = getDevice(id);
	
	if (device && removeDevice(device))
		return device;
	
	return NULL;
}

bool DeviceManager::removeDevice(VirtualDevice *device)
{
    bool found = false;

    for (int row = _deviceHierarchy.size() - 1; row >= 0; row--) {
        for (int col = 0; col < _deviceHierarchy[row]->size(); col++) {
            if (_deviceHierarchy[row]->at(col) == device) {
                _deviceHierarchy[row]->erase(_deviceHierarchy[row]->begin() + col);				
                found = true;
                break;
            }
        }

		// only delete upper rows if they are empty -> retain lower ones
        if (_deviceHierarchy[row]->empty() && row >= _deviceHierarchy.size() - 1) {
			std::vector<VirtualDevice*> *tmp = _deviceHierarchy[row];
            _deviceHierarchy.erase(_deviceHierarchy.begin() + row);
			
			delete tmp;
			tmp = NULL;
        }
    }

    buildDevices();

    return found;
}

void DeviceManager::buildDevices() {
    for (int row = _deviceHierarchy.size() - 1; row >= 0; row--) {
        for (int col = 0; col < _deviceHierarchy[row]->size(); col++) {
            _deviceHierarchy[row]->at(col)->resetAreas();

            for (int innerRow = _deviceHierarchy.size() - 1; innerRow > row; innerRow--) {
                for (int innerCol = 0; innerCol < _deviceHierarchy[innerRow]->size(); innerCol++) {
                    int start = _deviceHierarchy[innerRow]->at(innerCol)->getStartIndex();
                    int end = _deviceHierarchy[innerRow]->at(innerCol)->getEndIndex();

                    _deviceHierarchy[row]->at(col)->removeArea(start, end);
                }
            }
        }
    }
}

void DeviceManager::update()
{
	unsigned long currentMillis = millis();
	
	if (currentMillis - _lastUpdateMillis > 1000.0 / 60.0) { // update 60 times every second
		_device->clear();
	
		if (_onState) {
			for (int row = 0; row < _deviceHierarchy.size(); row++) {
				for (int col = 0; col < _deviceHierarchy[row]->size(); col++) {
					_deviceHierarchy[row]->at(col)->update(currentMillis - _lastUpdateMillis);
				}
			}
		}
	
		_device->update();
		
		_lastUpdateMillis = currentMillis;
	}
}

DeviceManager LEDDeviceManager;