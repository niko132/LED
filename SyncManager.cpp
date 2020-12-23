#include "SyncManager.h"

#include "DeviceManager.h"
#include "CustomEffect.h"

#include <string.h>

#include "ESPLogger.h"

SyncManager::SyncManager()
{

}

void SyncManager::begin(AsyncWebServer *server)
{
	_server = server;

	server->on("/get_syncable_devices", HTTP_GET, [this](AsyncWebServerRequest *request){
		AsyncJsonResponse *response = new AsyncJsonResponse();
		JsonObject root = response->getRoot().to<JsonObject>();

		for (std::map<String, std::vector<unsigned long>*>::iterator it = _syncableDevices.begin(); it != _syncableDevices.end(); it++) {
			JsonArray idArray = root.createNestedArray(it->first);

			for (int i = 0; i < it->second->size(); i++) {
				idArray.add(it->second->at(i));
			}
		}

		response->setLength();
		request->send(response);
	});

	if (_udp.listen(6789)) {
		Serial.print("UDP Listening on IP: ");
		Serial.println(WiFi.localIP());
		Serial.println("::UDP::");

		_udp.onPacket([this](AsyncUDPPacket packet) {
			MagicReader reader(packet.data(), packet.length());
			unsigned char type = 0;

			if (reader.read(&type)) {
				if (type == 0) { // device search request
					handleSearchRequest(&reader, &packet);
				} else if (type == 1) { // device search response
					handleSearchResponse(&reader, &packet);
				}else if (type == 11) { // led count request
					handleLedCountRequest(&reader, &packet);
				} else if (type == 12) { // led count response
					handleLedCountResponse(&reader, &packet);
				} else if (type == 13) { // sync data
					handleSyncData(&reader, &packet);
				} else if (type == 14) { // effect name
					handleEffectName(&reader, &packet);
				} else if (type == 15) { // effect data
					handleEffectData(&reader, &packet);
				} else if (type == 16) { // sync config
					handleSyncConfig(&reader, &packet);
				} else if (type == 17) { // raw pixel data
					handlePixelData(&reader, &packet);
				}
			}
		});
	} else {
		Serial.println("Failed listening for UDP");
	}
}

void SyncManager::handleSearchRequest(MagicReader *reader, AsyncUDPPacket *packet)
{
	unsigned char type = 1; // device search response
	unsigned int deviceCount = LEDDeviceManager.getDeviceCount();

	MagicWriter writer;
	writer.write(type);
	writer.write(deviceCount);

	for (int i = 0; i < deviceCount; i++) {
		VirtualDevice *device = LEDDeviceManager.getDeviceAt(i);
		unsigned long id = device->getId();

		writer.write(id);
	}

	unsigned char *buf = NULL;
	unsigned int length = 0;
	buf = writer.getData(&length);

	packet->write(buf, length);
}

void SyncManager::handleSearchResponse(MagicReader *reader, AsyncUDPPacket *packet)
{
	unsigned int deviceCount = 0;

	if (!reader->read(&deviceCount))
		return;

	if (deviceCount > 0) {
		String ip = packet->remoteIP().toString();
		std::vector<unsigned long> *ids = NULL;

		std::map<String, std::vector<unsigned long>*>::iterator it = _syncableDevices.find(ip);
		if (it != _syncableDevices.end()) {
			ids = it->second;
		} else {
			ids = new std::vector<unsigned long>();
			_syncableDevices[ip] = ids;
		}

		for (int i = 0; i < deviceCount; i++) {
			unsigned long id;

			if (!reader->read(&id)) {
				return;
			}

			ids->push_back(id);
		}
	}
}

void SyncManager::handleLedCountRequest(MagicReader *reader, AsyncUDPPacket *packet)
{
	unsigned long id;

	if (!reader->read(&id))
		return;

	Logger.println("Received led count request");

	VirtualDevice *device = LEDDeviceManager.getDevice(id);

	unsigned char type = 12; // response led count
	unsigned int ledCount = 0;

	if (device)
		ledCount = device->getLedCount();

	MagicWriter writer;
	writer.write(type);
	writer.write(id);
	writer.write(ledCount);

	unsigned char *buf = NULL;
	unsigned int length = 0;
	buf = writer.getData(&length);

	Logger.println("Sending led count response...");

	packet->write(buf, length);
}

void SyncManager::handleLedCountResponse(MagicReader *reader, AsyncUDPPacket *packet)
{
	unsigned long id;
	unsigned int ledCount = 0;

	if (!reader->read(&id) || !reader->read(&ledCount))
		return;

	Logger.println("Received led count response");

	IPAddress ip = packet->remoteIP();

	for (std::map<VirtualDevice*, std::vector<SyncDevice*>*>::iterator it = _syncs.begin(); it != _syncs.end(); it++) {
		for (std::vector<SyncDevice*>::iterator it2 = it->second->begin(); it2 != it->second->end(); it2++) {
			SyncDevice* syncDevice = *it2;

			if (syncDevice->getIp() == ip && syncDevice->getId() == id)
				syncDevice->setLedCount(ledCount);
				// dont break -> there might be more than one of this device
		}
	}
}

void SyncManager:: handleSyncData(MagicReader *reader, AsyncUDPPacket *packet)
{
	unsigned long id;
	double posStart;
	double posEnd;
	unsigned long timeOffset;

	if (!reader->read(&id) || !reader->read(&posStart) || !reader->read(&posEnd) || !reader->read(&timeOffset))
		return;

	VirtualDevice *device = LEDDeviceManager.getDevice(id);

	if (device) {
		device->setPosStart(posStart);
		device->setPosEnd(posEnd);
		device->syncTimeOffset(timeOffset);
	}
}

void SyncManager::handleEffectName(MagicReader *reader, AsyncUDPPacket *packet)
{
	unsigned long id;
	String name;

	if(!reader->read(&id) || !reader->read(&name))
		return;

	Logger.println("Received effect name");

	VirtualDevice *device = LEDDeviceManager.getDevice(id);

	if (device) {
		device->syncEffect(name);
	}
}

void SyncManager::handleEffectData(MagicReader *reader, AsyncUDPPacket *packet)
{
	unsigned long id;
	unsigned char *data = NULL;
	unsigned int length = 0;

	if (!reader->read(&id))
		return;

	data = reader->getRemainingData(&length);

	Logger.println("Received effect data");

	VirtualDevice *device = LEDDeviceManager.getDevice(id);

	if (device) {
		device->setEffect(data, length);
	}
}

void SyncManager::handleSyncConfig(MagicReader *reader, AsyncUDPPacket *packet)
{
	unsigned long id;
	int mode;

	if (!reader->read(&id) || !reader->read(&mode))
		return;

	Logger.println("Received sync config");

	VirtualDevice *device = LEDDeviceManager.getDevice(id);

	if (device) {
		device->setSyncMode(mode);
	}
}

void SyncManager::handlePixelData(MagicReader *reader, AsyncUDPPacket *packet)
{
	unsigned long id;
	unsigned int length = 0;
	unsigned char* pixelData = NULL;

	if (!reader->read(&id))
		return;

	pixelData = reader->getRemainingData(&length);

	VirtualDevice *device = LEDDeviceManager.getDevice(id);

	if (device) {
		device->syncPixelData(pixelData, length);
	}
}

void SyncManager::startSync(VirtualDevice *device, IPAddress ip, unsigned long id, int mode)
{
	std::vector<SyncDevice*> *syncedList = NULL;

	std::map<VirtualDevice*, std::vector<SyncDevice*>*>::iterator it = _syncs.find(device);
	if (it != _syncs.end()) {
		syncedList = it->second;
	} else {
		syncedList = new std::vector<SyncDevice*>();
		_syncs[device] = syncedList;
	}

	SyncDevice *syncDevice = NULL;

	// check if we are already synced
	for (std::vector<SyncDevice*>::iterator it = syncedList->begin(); it != syncedList->end(); it++) {
		if ((*it)->getIp() == ip && (*it)->getId() == id) {
			syncDevice = *it;
		}
	}

	if (syncDevice == NULL) {
		if (ip == WiFi.localIP()) {
			syncDevice = new InternalSync(id);
		} else {
			syncDevice = new ExternalSync(ip, id, &_udp);
		}

		syncedList->push_back(syncDevice);
	}

	syncDevice->setSyncConfig(WiFi.localIP(), device->getId(), mode);
	syncDevice->retrieveLedCount();

	deviceChanged(device);
}

void SyncManager::deviceChanged(VirtualDevice *device)
{
	std::map<VirtualDevice*, std::vector<SyncDevice*>*>::iterator it = _syncs.find(device);
	if (it != _syncs.end()) {
		VirtualDevice *master = it->first;

		/*
		int effectIndex = master->getEffectIndex();

		if (effectIndex < 0) { // Custom Effect
			CustomEffect *effect = (CustomEffect*) master->getEffect();

			unsigned char *buf = NULL;
			unsigned int length = 0;

			buf = effect->serialize(&length);

			Serial.println("Serilized effect c: " + String(length));

			for (std::vector<SyncDevice*>::iterator it2 = it->second->begin(); it2 != it->second->end(); it2++) {
				SyncDevice* syncDevice = *it2;

				Serial.println("Syncing Effect: " + String(syncDevice->getId()));

				syncDevice->setEffect(buf, length);
			}

			delete[] buf;
		} else { // Normal Effect
			for (std::vector<SyncDevice*>::iterator it2 = it->second->begin(); it2 != it->second->end(); it2++) {
				SyncDevice* syncDevice = *it2;

				Serial.println("Syncing Effect n: " + String(syncDevice->getId()));

				syncDevice->setEffect(effectIndex);
			}
		}
		*/

		String effectName = master->getEffect()->getName();

		for (std::vector<SyncDevice*>::iterator it2 = it->second->begin(); it2 != it->second->end(); it2++) {
			SyncDevice* syncDevice = *it2;

			syncDevice->setEffect(effectName);
		}


		// TODO: resend effect and palette settings
		// TODO: resend sync config
	}
}

void SyncManager::doSync(VirtualDevice *master, std::vector<SyncDevice*> *slaves)
{
	int totalCount = master->getLedCount();

	for (std::vector<SyncDevice*>::iterator it = slaves->begin(); it != slaves->end(); it++) {
		totalCount += (*it)->getLedCount();
	}

	int endCount = master->getLedCount();

	double posStart = 0.0;
	double posEnd = (double) endCount / totalCount;

	master->setPosStart(posStart);
	master->setPosEnd(posEnd);

	// unsigned long timeOffset = master->getTimeOffset();
	unsigned long timeOffset = millis();

	for (std::vector<SyncDevice*>::iterator it = slaves->begin(); it != slaves->end(); it++) {
		posStart = posEnd;

		endCount += (*it)->getLedCount();
		posEnd = (double) endCount / totalCount;

		(*it)->doSync(posStart, posEnd, timeOffset);
	}
}

void SyncManager::searchSyncableDevices()
{
	for (std::map<String, std::vector<unsigned long>*>::iterator it = _syncableDevices.begin(); it != _syncableDevices.end(); it++) {
		delete it->second;
		it->second = NULL;
	}

	_syncableDevices.clear();

	// first add every local device
	if (LEDDeviceManager.getDeviceCount() > 0) {
		std::vector<unsigned long> *ids = new std::vector<unsigned long>();

		for (int i = 0; i < LEDDeviceManager.getDeviceCount(); i++) {
			ids->push_back(LEDDeviceManager.getDeviceAt(i)->getId());
		}

		_syncableDevices[WiFi.localIP().toString()] = ids;
	}

	unsigned char *buf = new unsigned char[MAGIC_LENGTH + 1];
	unsigned int writeOff = 0;

	memcpy(&buf[writeOff], MAGIC, MAGIC_LENGTH);
	writeOff += MAGIC_LENGTH;

	// TODO: use consistent protocol
	buf[writeOff++] = 0;

	// send one byte
	_udp.broadcastTo(buf, writeOff, 6789);

	delete[] buf;
}

void SyncManager::update()
{
	unsigned long currentMillis = millis();

	for (std::map<VirtualDevice*, std::vector<SyncDevice*>*>::iterator it = _syncs.begin(); it != _syncs.end(); it++) {
		VirtualDevice* master = it->first;

		bool shouldSync = false;

		for (std::vector<SyncDevice*>::iterator it2 = it->second->begin(); it2 != it->second->end(); it2++) {
			SyncDevice* syncDevice = *it2;

			if (syncDevice->needsSync()) {
				if (syncDevice->getLedCount() > 0) {
					shouldSync = true;
					break;
				} else {
					syncDevice->retrieveLedCount();
				}
			}
		}

		if (shouldSync) {
			doSync(master, it->second);
		}
	}

	if (currentMillis - _lastDeviceSearchMillis > 5000.0) { // search every 5 seconds
		searchSyncableDevices();
		_lastDeviceSearchMillis = currentMillis;
	}
}

SyncManager LEDSyncManager;
