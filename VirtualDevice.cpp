#include "VirtualDevice.h"

#include "PaletteManager.h"
#include "SyncManager.h"
#include "EffectManager.h"

#include "ColorCycle.h"
#include "ColorFade.h"
#include "StaticColor.h"
#include "Snake.h"
#include "PingPong.h"
#include "CustomEffect.h"

#include <FastLED.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>

VirtualDevice::VirtualDevice(PhysicalDevice *device, int startIndex, int endIndex, int mode)
{
    _device = device;
    _startIndex = startIndex;
    _endIndex = endIndex;
    _mode = mode;

    //TODO: update id generation
    _id = ESP.getCycleCount();
	
	/*
	// generate default rainbow palette
	std::vector<ColorKey> colors;
	
	for (int i = 0; i < 256; i++) {
		double pos = (double) i / 255.0;
		CRGB color = CHSV(i, 255, 255);
		
		colors.push_back({pos, color});
	}
			
    _palette = new Palette(colors);
	*/
	
	_palette = Palettes.getPalette("rainbow", getLedRangeCount());
	

    // default effect
	
	setEffect(0);
	
    // _effect = new ColorCycle(_palette);
	// _effect = new ColorFade();
	// _effect = new StaticColor();
	// _effect = new Snake();

    resetAreas();
}

VirtualDevice::VirtualDevice(PhysicalDevice *device, unsigned long id)
{
	_device = device;
	
	String filename = "vd/" + String(id) + ".vd";
	File file = SPIFFS.open(filename, "r");
	
	if (!file) { // just create a default device
		_id = id;
		_startIndex = 0;
		_endIndex = device->getLedCount();
		_mode = 0;
		_posStart = 0.0;
		_posEnd = 1.0;
		_effectIndex = 0;
	} else { // otherwise read from file
		file.readBytes((char*) &_id, sizeof(_id));
		file.readBytes((char*) &_startIndex, sizeof(_startIndex));
		file.readBytes((char*) &_endIndex, sizeof(_endIndex));
		file.readBytes((char*) &_mode, sizeof(_mode));
		file.readBytes((char*) &_posStart, sizeof(_posStart));
		file.readBytes((char*) &_posEnd, sizeof(_posEnd));
		file.readBytes((char*) &_effectIndex, sizeof(_effectIndex));
		
		file.close();
	}
	
	_palette = Palettes.getPalette("rainbow", getLedRangeCount());
	setEffect(_effectIndex);
	resetAreas();
}

VirtualDevice::~VirtualDevice()
{
	for (int i = 0; i < _subIndices.size(); i++) {
		delete[] _subIndices[i];
	}
	_subIndices.clear();
	
	if (_effect) {
		delete _effect;
		_effect = NULL;
	}
	
	if (_palette) {
		// delete _palette; // _palette is retrieved from PaletteManager -> don't delete it
		_palette = NULL;
	}
	
	if (_server) {
		// already deletes the handler instances
		if (_effectHandler) {
			_server->removeHandler(_effectHandler);
			_effectHandler = NULL;
		}
		
		if (_posHandler) {
			_server->removeHandler(_posHandler);
			_posHandler = NULL;
		}
		
		if (_syncHandler) {
			_server->removeHandler(_syncHandler);
			_syncHandler = NULL;
		}
	}
	
	// TODO: delete device from SPIFFS
}

void VirtualDevice::begin(AsyncWebServer *server)
{
	Serial.println("Virtual Device " + String(_id) + ": begin");
	
	_server = server;
	
	_effectHandler = new AsyncCallbackJsonWebHandler("/" + String(_id) + "/set_effect", [this](AsyncWebServerRequest *request, JsonVariant &json) {
		JsonObject jsonObj = json.as<JsonObject>();
		
		int index = -1; // mandatory
		
		// TODO: implement effect by name
		if (jsonObj.containsKey("index")) {
			index = jsonObj["index"].as<int>();
			
			setEffect(index);
			
			LEDSyncManager.deviceChanged(this);
			
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
		
		
			LEDSyncManager.startSync(this, ipAddress, id);
			
		
			request->send(200, "text/plain", "Start sync: " + ip + " - " + String(id));
		} else {
			request->send(400, "text/plain", "ip, id needed");
		}
	});
	
	AsyncCallbackJsonWebHandler *customHandler = new AsyncCallbackJsonWebHandler("/" + String(_id) + "/custom_effect", [this](AsyncWebServerRequest *request, JsonVariant &json) {
		String effectJson;
		serializeJson(json, effectJson);
		
		// delete old effect
		if (_effect) {
			delete _effect;
			_effect = NULL;
		}
		
		// create the new one
		_effect = new CustomEffect(_palette, effectJson);
		
		_effectIndex = LEDEffectManager.createEffect((CustomEffect*) _effect);
		Serial.println("New Effect: " + String(_effectIndex));
		
		
		LEDSyncManager.deviceChanged(this);
		
		request->send(200, "text/plain", "Ok");
	}, 5 * 1024);
	
	server->addHandler(_effectHandler);
	server->addHandler(_posHandler);
	server->addHandler(_syncHandler);
	server->addHandler(customHandler);
	
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
	LEDSyncManager.deviceChanged(this);
	serialize();
}

void VirtualDevice::setEndIndex(int endIndex)
{
    _endIndex = endIndex;
	LEDSyncManager.deviceChanged(this);
	serialize();
}

void VirtualDevice::setMode(int mode)
{
    _mode = mode;
	LEDSyncManager.deviceChanged(this);
	serialize();
}

void VirtualDevice::setEffect(int index)
{
	if (_effect) {
		delete _effect;
		_effect = NULL;
	}
	
	/*
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
	
	_effectIndex = index;
	LEDSyncManager.deviceChanged(this);
	*/
	
	_effect = LEDEffectManager.getEffectAt(index, _palette);
	
	if (_effect) {
		_effectIndex = index;
	} else { // load default effect
		_effect = new ColorFade(_palette);
		_effectIndex = 0;
	}
	
	serialize();
}

void VirtualDevice::setEffect(unsigned char *buf, unsigned int length)
{
	if (_effect) {
		delete _effect;
		_effect = NULL;
	}
	
	_effect = new CustomEffect(_palette, buf, length);
	
	_effectIndex = -1;
	
	serialize();
}

void VirtualDevice::setTimeValue(double val)
{
	_lastTimeValue = val;
}

void VirtualDevice::setPosStart(double posStart)
{
	_posStart = posStart;
	serialize();
}

void VirtualDevice::setPosEnd(double posEnd)
{
	_posEnd = posEnd;
	serialize();
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
			int end = _subIndices[i][1];
			_subIndices[i][1] = startIndex;
			
            int *b = new int[2] {endIndex, end};
            _subIndices.insert(_subIndices.begin() + i + 1, b);
        } else if (startIndex <= _subIndices[i][0] && endIndex >= _subIndices[i][1]) {
            int *tmp = _subIndices[i];
			_subIndices.erase(_subIndices.begin() + i);
			
			delete[] tmp;
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

int VirtualDevice::getCount()
{
	if (_mode == 0)
		return getLedRangeCount();
	
	return getLedCount();
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

int VirtualDevice::getEffectIndex()
{
	return _effectIndex;
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

double VirtualDevice::getLastTimeValue()
{
	return _lastTimeValue;
}

Effect* VirtualDevice::getEffect()
{
	return _effect;
}

void VirtualDevice::serialize()
{
	String filename = "vd/" + String(_id) + ".vd";
	File file = SPIFFS.open(filename, "w");
	
	file.write((unsigned char*) &_id, sizeof(_id));
	file.write((unsigned char*) &_startIndex, sizeof(_startIndex));
	file.write((unsigned char*) &_endIndex, sizeof(_endIndex));
	file.write((unsigned char*) &_mode, sizeof(_mode));
	file.write((unsigned char*) &_posStart, sizeof(_posStart));
	file.write((unsigned char*) &_posEnd, sizeof(_posEnd));
	file.write((unsigned char*) &_effectIndex, sizeof(_effectIndex));
	
	file.close();
}

void VirtualDevice::update(unsigned long delta)
{
	unsigned long duration = (unsigned long) (_effect->getDuration() * 1000.0);
	
	delta = delta % duration; // 5 secs
	double timeValue = _lastTimeValue + (double) delta / duration;
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
}