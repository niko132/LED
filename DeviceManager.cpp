#include "DeviceManager.h"

#include "ColorCycle.h"
#include "ColorFade.h"
#include "StaticColor.h"
#include "Snake.h"
#include "PingPong.h"
#include "CustomEffect.h"

#include <AsyncJson.h>
#include <ArduinoJson.h>

DeviceManager::DeviceManager(PhysicalDevice *device)
{
    _device = device;
	
	int ledCount = device->getLedCount();
	addDevice(0, ledCount, 0, 0);
}

DeviceManager::DeviceManager(int ledCount)
{
    Serial.println("DeviceManager Constructor: " + String(ledCount));

    _device = new PhysicalDevice(ledCount);

	addDevice(0, ledCount, 0, 0);
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
		
		unsigned long id = 0;
		
		if (jsonObj.containsKey("id")) {
			id = jsonObj["id"].as<unsigned long>();
			
			Serial.println("Remove: " + String(id));
	
			VirtualDevice* device = removeDevice(id);
			if (device) {
				delete device;
				device = NULL;
			}
			
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
	
	server->on("/get_syncable_devices", HTTP_GET, [this](AsyncWebServerRequest *request){		
		AsyncJsonResponse *response = new AsyncJsonResponse();
		JsonObject root = response->getRoot().to<JsonObject>();
		
		for (std::map<String, std::vector<unsigned long>>::iterator it = _syncableDevices.begin(); it != _syncableDevices.end(); it++) {
			JsonArray idArray = root.createNestedArray(it->first);
			
			for (int i = 0; i < it->second.size(); i++) {
				idArray.add(it->second[i]);
			}
		}
		
		response->setLength();
		request->send(response);
	});
	
	server->on("/", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(SPIFFS, "/index.html", String());
	});
	
	if (_udp.listen(6789)) {
		Serial.print("UDP Listening on IP: ");
        Serial.println(WiFi.localIP());
		
        _udp.onPacket([this](AsyncUDPPacket packet) {
			/*
            Serial.print("UDP Packet Type: ");
            Serial.print(packet.isBroadcast()?"Broadcast":packet.isMulticast()?"Multicast":"Unicast");
            Serial.print(", From: ");
            Serial.print(packet.remoteIP());
            Serial.print(":");
            Serial.print(packet.remotePort());
            Serial.print(", To: ");
            Serial.print(packet.localIP());
            Serial.print(":");
            Serial.print(packet.localPort());
            Serial.print(", Length: ");
            Serial.print(packet.length());
            Serial.print(", Data: ");
            Serial.write(packet.data(), packet.length());
            Serial.println();
			*/
            // reply to the client
            // packet.printf("Got %u bytes of data", packet.length());
			
			if (packet.data()[0] == 0) { // device search request
				unsigned int deviceCount = getDeviceCount();
			
				// 1 op-byte + 1 * ul count + count * ul
				unsigned char *buf = new unsigned char[1 + sizeof(unsigned int) + deviceCount * sizeof(unsigned long)];
				unsigned int index = 0;
				
				buf[index] = 1;
				index += 1;
				
				memcpy(&buf[index], &deviceCount, sizeof(deviceCount));
				index += sizeof(deviceCount);
				
				for (int i = 0; i < deviceCount; i++) {
					VirtualDevice *device = getDeviceAt(i);
					unsigned long id = device->getId();
					
					memcpy(&buf[index], &id, sizeof(id));
					index += sizeof(id);
				}
				
				packet.write(buf, index);
				
				delete[] buf;
			} else if (packet.data()[0] == 1 && packet.length() >= 5) { // device search reply
				unsigned int index = 1;
				
				unsigned int deviceCount;
				memcpy(&deviceCount, &packet.data()[index], sizeof(deviceCount));
				index += sizeof(deviceCount);
				
				std::vector<unsigned long> ids;
				
				for (int i = 0; i < deviceCount; i++) {
					unsigned long id;
					memcpy(&id, &packet.data()[index], sizeof(unsigned long));
					index += sizeof(id);
					
					ids.push_back(id);
				}
				
				if (!ids.empty()) {
					_syncableDevices[packet.remoteIP().toString()] = ids;
				}
				
			} else if (packet.length() >= 5) { // let the device handle this message
				unsigned long id;
				memcpy(&id, &packet.data()[1], sizeof(id));
				
				VirtualDevice *device = getDevice(id);
				
				if (device) {
					device->receivedUdpMessage(&packet);
				}
			}
				
			/*
			} else if (packet.data()[0] == 11) {
				int index = 1;
				
				unsigned long id;
				memcpy(&id, &packet.data()[index], sizeof(id));
				index += sizeof(id);
				
				double val;
				memcpy(&val, &packet.data()[index], sizeof(val));
				index += sizeof(val);
				
				
				unsigned char buf[50];
				int cnt = 0;
		
				buf[cnt] = 12;
				cnt += 1;
		
				memcpy(&buf[cnt], &id, sizeof(id));
				cnt += sizeof(id);
		
				memcpy(&buf[cnt], &val, sizeof(val));
				cnt += sizeof(val);
				
				
				packet.write(buf, cnt);
				
				VirtualDevice* device = getDevice(id);
				if (device) {
					device->setPosStart(0.5);
					device->setPosEnd(1.0);
					
					device->setTimeValue(val);
				}
			} else if (packet.data()[0] == 12) {
				int index = 1;
				
				unsigned long id;
				memcpy(&id, &packet.data()[index], sizeof(id));
				index += sizeof(id);
				
				double val;
				memcpy(&val, &packet.data()[index], sizeof(val));
				index += sizeof(val);
				
				Serial.println("Got UDP Data: " + String(id) + " " + String(val));
			}
			*/
        });
	}
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

    VirtualDevice *newDevice = new VirtualDevice(_device, startIndex, endIndex, mode, &_udp);
	
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

void DeviceManager::searchRemoteDevices()
{
	_syncableDevices.clear();
	
	unsigned char buf;
	
	// TODO: use consistent protocol
	buf = 0;
	
	// send one byte
	_udp.broadcastTo(&buf, 1, 6789);
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
	
	if (currentMillis - _lastDeviceSearchMillis > 5000.0) { // search every 5 seconds
		searchRemoteDevices();
		
		_lastDeviceSearchMillis = currentMillis;
	}
}