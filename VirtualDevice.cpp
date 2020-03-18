#include "VirtualDevice.h"

#include "ColorCycle.h"
#include "ColorFade.h"
#include "StaticColor.h"
#include "Snake.h"
#include "PingPong.h"

#include <FastLED.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>

VirtualDevice::VirtualDevice(PhysicalDevice *device, int startIndex, int endIndex, int mode, AsyncUDP *udp)
{
    _device = device;
    _startIndex = startIndex;
    _endIndex = endIndex;
    _mode = mode;

    //TODO: update id generation
    _id = ESP.getCycleCount();
	
	// generate default rainbow palette
	std::vector<ColorKey> colors;
	
	for (int i = 0; i < 256; i++) {
		double pos = (double) i / 255.0;
		CRGB color = CHSV(i, 255, 255);
		
		colors.push_back({pos, color});
	}
			
    _palette = new Palette(colors);
	

    // default effect
	
    _effect = new ColorCycle(_palette);
	// _effect = new ColorFade();
	// _effect = new StaticColor();
	// _effect = new Snake();

    resetAreas();
	
	_udp = udp;
}

VirtualDevice::~VirtualDevice()
{
	if (_effect) {
		delete _effect;
		_effect = NULL;
	}
	
	if (_palette) {
		delete _palette;
		_palette = NULL;
	}
	
	if (_server) {
		// already deletes the handler instances
		_server->removeHandler(_effectHandler);
		_effectHandler = NULL;
		
		_server->removeHandler(_posHandler);
		_posHandler = NULL;
		
		_server->removeHandler(_syncHandler);
		_syncHandler = NULL;
	}
}

void VirtualDevice::begin(AsyncWebServer *server)
{
	Serial.println("Virtual Device " + String(_id) + ": begin");
	
	_server = server;
	
	_effectHandler = new AsyncCallbackJsonWebHandler("/" + String(_id) + "/set_effect", [this](AsyncWebServerRequest *request, JsonVariant &json) {
		JsonObject jsonObj = json.as<JsonObject>();
		
		int index = -1;
		
		// TODO: implement effect by name
		if (jsonObj.containsKey("index")) {
			index = jsonObj["index"].as<int>();
			
			setEffect(index);
			request->send(200, "text/plain", "Effect: " + String(index));
		} else {
			request->send(400, "text/plain", "index needed");
		}
	});
	
	_posHandler = new AsyncCallbackJsonWebHandler("/" + String(_id) + "/set_pos", [this](AsyncWebServerRequest *request, JsonVariant &json) {
		JsonObject jsonObj = json.as<JsonObject>();
		
		double posStart = -1;
		double posEnd = -1;
		
		if (jsonObj.containsKey("posStart") || jsonObj.containsKey("posEnd")) {
			if (jsonObj.containsKey("posStart")) {
				posStart = jsonObj["posStart"].as<double>();
				setPosStart(posStart);
			}
			
			if (jsonObj.containsKey("posEnd")) {
				posEnd = jsonObj["posEnd"].as<double>();
				setPosEnd(posEnd);
			}
			
			request->send(200, "text/plain", "PosStart: " + String(posStart) + " PosEnd: " + String(posEnd));
		} else {
			request->send(400, "text/plain", "posStart or posEnd needed");
		}
	});
	
	_syncHandler = new AsyncCallbackJsonWebHandler("/" + String(_id) + "/start_sync", [this](AsyncWebServerRequest *request, JsonVariant &json) {
		JsonObject jsonObj = json.as<JsonObject>();
		
		String ip = "";
		unsigned long id = 0;
		
		if (jsonObj.containsKey("ip") && jsonObj.containsKey("id")) {
			ip = jsonObj["ip"].as<String>();
			id = jsonObj["id"].as<unsigned long>();
			
			Serial.println("Sync: " + ip + " - " + String(id));
		
			IPAddress ipAddress;
			ipAddress.fromString(ip);
		
			_syncRequests.push_back({ipAddress, id});
		
			request->send(200, "text/plain", "Start sync: " + ip + " - " + String(id));
		} else {
			request->send(400, "text/plain", "ip, id needed");
		}
	});
	
	server->addHandler(_effectHandler);
	server->addHandler(_posHandler);
	server->addHandler(_syncHandler);
	
	server->on(String("/" + String(_id) + "/get_effects").c_str(), HTTP_GET, [this](AsyncWebServerRequest *request){		
		AsyncJsonResponse *response = new AsyncJsonResponse();
		JsonArray root = response->getRoot().to<JsonArray>();
		
		Effect* effects[] = {
			new ColorCycle(_palette),
			new ColorFade(_palette),
			new StaticColor(_palette),
			new Snake(_palette),
			new PingPong(_palette)
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
}

void VirtualDevice::setStartIndex(int startIndex)
{
    _startIndex = startIndex;
}

void VirtualDevice::setEndIndex(int endIndex)
{
    _endIndex = endIndex;
}

void VirtualDevice::setMode(int mode)
{
    _mode = mode;
}

void VirtualDevice::setEffect(int index)
{
	if (_effect) {
		delete _effect;
		_effect = NULL;
	}
	
	switch(index) {
		case 1:
			_effect = new ColorFade(_palette);
			break;
		case 2:
			_effect = new StaticColor(_palette);
			break;
		case 3:
			_effect = new Snake(_palette);
			break;
		case 4:
			_effect = new PingPong(_palette);
			break;
		default:
			_effect = new ColorCycle(_palette);
	}
}

void VirtualDevice::setTimeValue(double val)
{
	_lastTimeValue = val;
}

void VirtualDevice::setPosStart(double posStart)
{
	_posStart = posStart;
}

void VirtualDevice::setPosEnd(double posEnd)
{
	_posEnd = posEnd;
}

void VirtualDevice::resetAreas()
{
	for (int i = 0; i < _subIndices.size(); i++) {
		delete[] _subIndices[i];
		_subIndices[i] = NULL;
	}
    _subIndices.clear();
    _subIndices.push_back(new int[2] {_startIndex, _endIndex});
}

void VirtualDevice::removeArea(int startIndex, int endIndex)
{
    for (int i = _subIndices.size() - 1; i >= 0; i--) {
        if (startIndex <= _subIndices[i][0] && endIndex >= _subIndices[i][0] && endIndex < _subIndices[i][1]) {
            _subIndices[i][0] = endIndex;
        } else if (endIndex >= _subIndices[i][1] && startIndex <= _subIndices[i][1] && startIndex > _subIndices[i][0]) {
            _subIndices[i][1] = startIndex;
        } else if (startIndex > _subIndices[i][0] && endIndex < _subIndices[i][1]) {
            int *a = new int[2] {_subIndices[i][0], startIndex};
            int *b = new int[2] {endIndex, _subIndices[i][1]};

            _subIndices.erase(_subIndices.begin() + i);
            _subIndices.insert(_subIndices.begin() + i, a);
            _subIndices.insert(_subIndices.begin() + 1, b);
        } else if (startIndex <= _subIndices[i][0] && endIndex >= _subIndices[i][1]) {
            _subIndices.erase(_subIndices.begin() + i);
        }
    }
}

int VirtualDevice::getStartIndex()
{
    return _startIndex;
}

int VirtualDevice::getEndIndex()
{
    return _endIndex;
}

int VirtualDevice::getLedCount()
{
    int sum = 0;
	
	for (int i = 0; i < _subIndices.size(); i++) {
		sum += _subIndices[i][1] - _subIndices[i][0];
	}
	
	return sum;
}

int VirtualDevice::getLedRangeCount()
{
	return _endIndex - _startIndex;
}

int VirtualDevice::getMode()
{
    return _mode;
}

double VirtualDevice::getPosStart()
{
	return _posStart;
}

double VirtualDevice::getPosEnd()
{
	return _posEnd;
}

unsigned long VirtualDevice::getId()
{
    return _id;
}

void VirtualDevice::update(unsigned long delta)
{
	delta = delta % 5000; // 5 secs
	double timeValue = _lastTimeValue + (double) delta / 5000.0;
		
	timeValue -= (int) timeValue;
	
	int ledCount = 0;

	for (int i = 0; i < _subIndices.size(); i++) {
		for (int index = _subIndices[i][0]; index < _subIndices[i][1]; index++, ledCount++) {
			double posValue = 0.0;

			if (_mode == 0) {
				posValue = (double) (index - _startIndex) / (double) getLedRangeCount();
			} else if (_mode == 1) {
				posValue = (double) (ledCount) / (double) getLedCount();
			}
			
			// CAUTION: test!!!
			posValue = _posStart + posValue * (_posEnd - _posStart);

			CRGB color = _effect->update(timeValue, posValue);

			// post process color...

			unsigned char* p = &_device->getPixelBuf()[index * 3];
			p[0] = color.r;
			p[1] = color.g;
			p[2] = color.b;
		}
	}
		
	_lastTimeValue = timeValue;
	
	handleSyncRequests();
}

void VirtualDevice::handleSyncRequests()
{
	/*
	while(!_syncRequests.empty()) {
		SyncRequest request = _syncRequests[0];
		
		Serial.println("Handling " + request.ip.toString() + " " + String(request.id));
		
		unsigned char buf[50];
		int cnt = 0;
		
		buf[cnt] = 11;
		cnt += 1;
		
		memcpy(&buf[cnt], &request.id, sizeof(request.id));
		cnt += sizeof(request.id);
		
		memcpy(&buf[cnt], &_lastTimeValue, sizeof(_lastTimeValue));
		cnt += sizeof(_lastTimeValue);
		
		_udp->writeTo(buf, cnt, request.ip, 6789);

		// TODO: auto update start and end position
		setPosStart(0.0);
		setPosEnd(0.5);
		
		_syncRequests.erase(_syncRequests.begin());
	}
	*/
	
	while(!_syncRequests.empty()) {
		SyncRequest request = _syncRequests[0];
		Serial.println("Handling " + request.ip.toString() + " " + String(request.id));
		
		unsigned char *buf = new unsigned char[1 + 2 * sizeof(unsigned long)];
		unsigned int index = 0;
		
		buf[index] = 11; // request led count
		index += 1;
		
		memcpy(&buf[index], &request.id, sizeof(request.id));
		index += sizeof(request.id);
		
		memcpy(&buf[index], &_id, sizeof(_id));
		index += sizeof(_id);
		
		_udp->writeTo(buf, index, request.ip, 6789);
		
		delete[] buf;
		
		_syncRequests.erase(_syncRequests.begin());
	}
}

void VirtualDevice::receivedUdpMessage(AsyncUDPPacket *packet)
{
	unsigned char *data = packet->data();
	
	unsigned int index = 1 + sizeof(unsigned long); // skip op code and own id
	
	if (data[0] == 11) { // led count request
		unsigned long id;
		memcpy(&id, &data[index], sizeof(id));
		index += sizeof(id);
		
		
		unsigned char *buf = new unsigned char[1 + 2 * sizeof(unsigned long) + sizeof(int)];
		unsigned int index = 0;
		
		buf[index] = 12; // response led count
		index += 1;
		
		memcpy(&buf[index], &id, sizeof(id));
		index += sizeof(id);
		
		memcpy(&buf[index], &_id, sizeof(_id));
		index += sizeof(_id);
		
		int count;
		
		if (_mode == 0)
			count = getLedRangeCount();
		else
			count = getLedCount();
		
		memcpy(&buf[index], &count, sizeof(count));
		index += sizeof(count);
		
		packet->write(buf, index);
		
		delete[] buf;
	} else if (data[0] == 12) { // led count response
		unsigned long id;
		memcpy(&id, &data[index], sizeof(id));
		index += sizeof(id);
	
		int syncCount;
		memcpy(&syncCount, &data[index], sizeof(syncCount));
		index += sizeof(syncCount);
		
		
		SyncedDevice syncedDev = {packet->remoteIP(), id, syncCount};
		_syncedDevices.push_back(syncedDev);
		
		
		int ownCount;
		if (_mode == 0)
			ownCount = getLedRangeCount();
		else
			ownCount = getLedCount();
		
		int totalCount = ownCount;
		for (int i = 0; i < _syncedDevices.size(); i++) {
			SyncedDevice syncedDev = _syncedDevices[i];
			totalCount += syncedDev.ledCount;
		}
		
		int endCount = ownCount;
		
		double posStart = 0.0;
		double posEnd = (double) endCount / totalCount;
		
		setPosStart(posStart);
		setPosEnd(posEnd);
		
		unsigned char *buf = new unsigned char[1 + sizeof(unsigned long) + 3 * sizeof(double)];
		
		for (int i = 0; i < _syncedDevices.size(); i++) {
			SyncedDevice syncedDev = _syncedDevices[i];
			
			posStart = posEnd;
			
			endCount += syncedDev.ledCount;
			posEnd = (double) endCount / totalCount;
			
			
			unsigned int index = 0;
		
			buf[index] = 13; // response sync config
			index += 1;
		
			memcpy(&buf[index], &syncedDev.id, sizeof(syncedDev.id));
			index += sizeof(syncedDev.id);
		
			memcpy(&buf[index], &posStart, sizeof(posStart));
			index += sizeof(posStart);
		
			memcpy(&buf[index], &posEnd, sizeof(posEnd));
			index += sizeof(posEnd);
		
			memcpy(&buf[index], &_lastTimeValue, sizeof(_lastTimeValue));
			index += sizeof(_lastTimeValue);
		
			_udp->writeTo(buf, index, syncedDev.ip, 6789);
		}
		
		delete[] buf;
	} else if (data[0] == 13) {
		double posStart;
		memcpy(&posStart, &data[index], sizeof(posStart));
		index += sizeof(posStart);
		
		double posEnd;
		memcpy(&posEnd, &data[index], sizeof(posEnd));
		index += sizeof(posEnd);
		
		double timeVal;
		memcpy(&timeVal, &data[index], sizeof(timeVal));
		index += sizeof(timeVal);
		
		
		setPosStart(posStart);
		setPosEnd(posEnd);
		setTimeValue(timeVal);
	}
}