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

#include "KeepLengthAlgorithm.h"
#include "CollapseLengthAlgorithm.h"

#include "SyncTime.h"

#include "ESPLogger.h"

#include <FastLED.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>

VirtualDevice::VirtualDevice(PhysicalDevice *device, unsigned int startIndex, unsigned int endIndex, int mode)
{
    _device = device;
    _coverAlgorithm = new KeepLengthAlgorithm(startIndex, endIndex);
    // TODO: do it a bit more elegant
    _syncAlgorithm = new SyncTime(0, NULL, getLedCount());
    setMode(mode);

    //TODO: update id generation
    _id = ESP.getCycleCount();

	_palette = Palettes.getPalette("rainbow", getLedCount());

    // default effect
	setEffect("");

    _coverAlgorithm->resetCovered();
}

VirtualDevice::VirtualDevice(PhysicalDevice *device, unsigned long id)
{
	_device = device;

	String filename = "vd/" + String(id) + ".vd";
	File file = SPIFFS.open(filename, "r");

    unsigned int startIndex = 0;
    unsigned int endIndex = 0;
    int mode = 0;
    String effectName = "";

	if (!file) { // just create a default device
		_id = id;
		startIndex = 0;
		endIndex = device->getLedCount();
		mode = 0;
		_posStart = 0.0;
		_posEnd = 1.0;
		effectName = "";
	} else { // otherwise read from file
		file.readBytes((char*) &_id, sizeof(_id));
		file.readBytes((char*) &startIndex, sizeof(startIndex));
		file.readBytes((char*) &endIndex, sizeof(endIndex));
		file.readBytes((char*) &mode, sizeof(mode));
		file.readBytes((char*) &_posStart, sizeof(_posStart));
		file.readBytes((char*) &_posEnd, sizeof(_posEnd));

        effectName = file.readStringUntil((char) 0);

		file.close();
	}

    _coverAlgorithm = new KeepLengthAlgorithm(startIndex, endIndex);
    _syncAlgorithm = new SyncTime(0, NULL, getLedCount());
    setMode(mode);

	_palette = Palettes.getPalette("rainbow", getLedCount());
	setEffect(effectName);
	_coverAlgorithm->resetCovered();
}

VirtualDevice::~VirtualDevice()
{
    Effect *effect = _syncAlgorithm->getEffect();
    _syncAlgorithm->setEffect(NULL);

	if (effect) {
		delete effect;
		effect = NULL;
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

		String name = ""; // mandatory

		// TODO: implement effect by name
		if (jsonObj.containsKey("name")) {
			name = jsonObj["name"].as<String>();

			setEffect(name);

			LEDSyncManager.deviceChanged(this);

			request->send(200, "text/plain", "Effect: " + name);
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

		// create the new one
		Effect *effect = new CustomEffect(_palette, effectJson);

        Effect *oldEffect = _syncAlgorithm->setEffect(effect);

        // delete old effect
		if (oldEffect) {
			delete oldEffect;
			oldEffect = NULL;
		}

		LEDEffectManager.createEffect((CustomEffect*) effect);
		Serial.println("New Effect: " + effect->getName());

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
    _coverAlgorithm->setStartIndex(startIndex);
    _syncAlgorithm->setLength(getLedCount());
	LEDSyncManager.deviceChanged(this);
	serialize();
}

void VirtualDevice::setEndIndex(int endIndex)
{
    _coverAlgorithm->setEndIndex(endIndex);
    _syncAlgorithm->setLength(getLedCount());
	LEDSyncManager.deviceChanged(this);
	serialize();
}

void VirtualDevice::setMode(int mode)
{
    if (mode != _mode) {
        _mode = mode;

        unsigned int startIndex = 0;
        unsigned int endIndex = 0;

        if (_coverAlgorithm) {
            startIndex = _coverAlgorithm->getStartIndex();
            endIndex = _coverAlgorithm->getEndIndex();

            delete _coverAlgorithm;
            _coverAlgorithm = NULL;
        }

        if (mode == 1) {
            _coverAlgorithm = new CollapseLengthAlgorithm(startIndex, endIndex);
        } else {
            _coverAlgorithm = new KeepLengthAlgorithm(startIndex, endIndex);
        }

        _syncAlgorithm->setLength(getLedCount());

    	LEDSyncManager.deviceChanged(this);
    	serialize();
    }
}

void VirtualDevice::setEffect(String name)
{
    Effect *effect = LEDEffectManager.getEffect(name, _palette);

    if (!effect) { // load default effect
		effect = LEDEffectManager.getDefaultEffect(_palette);
	}

    effect = _syncAlgorithm->setEffect(effect);

    if (effect) {
        delete effect;
        effect = NULL;
    }

	serialize();
}

void VirtualDevice::setEffect(unsigned char *buf, unsigned int length)
{
	Effect *effect = new CustomEffect(_palette, buf, length);
	effect = _syncAlgorithm->setEffect(effect);

    if (effect) {
        delete effect;
        effect = NULL;
    }

	serialize();
}

void VirtualDevice::setTimeOffset(unsigned long timeOffset)
{
	_syncAlgorithm->setTimeOffset(timeOffset);
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

void VirtualDevice::resetCovered()
{
	_coverAlgorithm->resetCovered();
}

void VirtualDevice::addCovered(unsigned int startIndex, unsigned int endIndex)
{
    _coverAlgorithm->addCovered(startIndex, endIndex);
}

unsigned int VirtualDevice::getStartIndex()
{
    return _coverAlgorithm->getStartIndex();
}

unsigned int VirtualDevice::getEndIndex()
{
    return _coverAlgorithm->getEndIndex();
}

unsigned int VirtualDevice::getLedCount()
{
	return _coverAlgorithm->getLedCount();
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

unsigned long VirtualDevice::getTimeOffset()
{
	return _syncAlgorithm->getTimeOffset();
}

Effect* VirtualDevice::getEffect()
{
	return _syncAlgorithm->getEffect();
}

void VirtualDevice::serialize()
{
	String filename = "vd/" + String(_id) + ".vd";
	File file = SPIFFS.open(filename, "w");

    unsigned int startIndex = _coverAlgorithm->getStartIndex();
    unsigned int endIndex = _coverAlgorithm->getEndIndex();

	file.write((unsigned char*) &_id, sizeof(_id));
	file.write((unsigned char*) &startIndex, sizeof(startIndex));
	file.write((unsigned char*) &endIndex, sizeof(endIndex));
	file.write((unsigned char*) &_mode, sizeof(_mode));
	file.write((unsigned char*) &_posStart, sizeof(_posStart));
	file.write((unsigned char*) &_posEnd, sizeof(_posEnd));

    String effectName = "";

    if (getEffect()) {
        effectName = getEffect()->getName();
    }

    file.write((unsigned char*) effectName.c_str(), effectName.length() + 1);

	file.close();
}

void VirtualDevice::update()
{
    Effect *effect = _syncAlgorithm->getEffect();

	unsigned long duration = (unsigned long) (effect->getDuration() * 1000.0);

    unsigned long current = millis() - _syncAlgorithm->getTimeOffset();
    current = current % duration; // 5 secs

	double timeValue = (double) current / duration;
	timeValue -= (int) timeValue;

    int currIndex = -1; // use -1 to start at the first index in the loop
    double currFrac = 0.0;
    int relIndex = 0;

    while (_coverAlgorithm->nextIndex(currIndex, &currIndex, &currFrac)) {
        double posValue = _posStart + currFrac * (_posEnd - _posStart);
        posValue -= (int) posValue;

        CRGB color = _syncAlgorithm->updatePixel(timeValue, posValue, relIndex++, effect);

        unsigned char *p = &_device->getPixelBuf()[currIndex * 3];
        p[0] = color.r;
        p[1] = color.g;
        p[2] = color.b;
    }
}
