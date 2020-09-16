#include "SyncManager.h"

#include "DeviceManager.h"
#include "CustomEffect.h"

#include <string.h>

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
			Serial.println("Received Packet: " + String(packet.length()));

			if (packet.length() > MAGIC_LENGTH && memcmp(packet.data(), MAGIC, MAGIC_LENGTH) == 0) {
				unsigned int readOff = MAGIC_LENGTH; // ignore the first bytes

				unsigned char type = packet.data()[readOff++];

				Serial.println("Correct Magic");
				Serial.println("Type: " + String(type));

				// TODO: implement using switch

				if (type == 0) { // device search request
					handleSearchRequest(&packet, readOff);
				} else if (type == 1) { // device search response
					handleSearchResponse(&packet, readOff);
				}else if (type == 11) { // led count request
					handleLedCountRequest(&packet, readOff);
				} else if (type == 12) { // led count response
					handleLedCountResponse(&packet, readOff);
				} else if (type == 13) { // sync config
					handleSyncConfig(&packet, readOff);
				} else if (type == 14) { // effect index
					handleEffectIndex(&packet, readOff);
				} else if (type == 15) { // effect data
					handleEffectData(&packet, readOff);
				}
			}
		});
	} else {
		Serial.println("Failed listening for UDP");
	}
}

void SyncManager::handleSearchRequest(AsyncUDPPacket *packet, unsigned int readOff)
{
	unsigned int deviceCount = LEDDeviceManager.getDeviceCount();

	// 1 op-byte + 1 * ul count + count * ul
	unsigned char *buf = new unsigned char[MAGIC_LENGTH + 1 + sizeof(unsigned int) + deviceCount * sizeof(unsigned long)];
	unsigned int writeOff = 0;

	memcpy(&buf[writeOff], MAGIC, MAGIC_LENGTH);
	writeOff += MAGIC_LENGTH;

	buf[writeOff] = 1;
	writeOff += 1;

	memcpy(&buf[writeOff], &deviceCount, sizeof(deviceCount));
	writeOff += sizeof(deviceCount);

	for (int i = 0; i < deviceCount; i++) {
		VirtualDevice *device = LEDDeviceManager.getDeviceAt(i);
		unsigned long id = device->getId();

		memcpy(&buf[writeOff], &id, sizeof(id));
		writeOff += sizeof(id);
	}

	packet->write(buf, writeOff);

	delete[] buf;
}

void SyncManager::handleSearchResponse(AsyncUDPPacket *packet, unsigned int readOff)
{
	if (packet->length() <= sizeof(unsigned int) + readOff)
		return;

	Serial.println("Handling Search Response");

	unsigned int deviceCount;
	memcpy(&deviceCount, &(packet->data()[readOff]), sizeof(deviceCount));
	readOff += sizeof(deviceCount);

	Serial.println("Found " + String(deviceCount) + " new Devices");

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
			memcpy(&id, &packet->data()[readOff], sizeof(unsigned long));
			readOff += sizeof(id);

			ids->push_back(id);
		}
	}
}

void SyncManager::handleLedCountRequest(AsyncUDPPacket *packet, unsigned int readOff)
{
	unsigned long id;
	memcpy(&id, &packet->data()[readOff], sizeof(id));
	readOff += sizeof(id);

	VirtualDevice *device = LEDDeviceManager.getDevice(id);

	unsigned char *buf = new unsigned char[MAGIC_LENGTH + 1 + sizeof(unsigned long) + sizeof(int)];
	unsigned int writeOff = 0;

	memcpy(&buf[writeOff], MAGIC, MAGIC_LENGTH);
	writeOff += MAGIC_LENGTH;

	buf[writeOff++] = 12; // response led count

	memcpy(&buf[writeOff], &id, sizeof(id));
	writeOff += sizeof(id);

	// respond with -1 if the device doesnt exist anymore
	int count = -1;

	if (device)
		count = device->getLedCount();

	memcpy(&buf[writeOff], &count, sizeof(count));
	writeOff += sizeof(count);

	packet->write(buf, writeOff);

	delete[] buf;
}

void SyncManager::handleLedCountResponse(AsyncUDPPacket *packet, unsigned int readOff)
{
	unsigned long id;
	memcpy(&id, &packet->data()[readOff], sizeof(id));
	readOff += sizeof(id);

	int ledCount;
	memcpy(&ledCount, &packet->data()[readOff], sizeof(ledCount));
	readOff += sizeof(ledCount);

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

void SyncManager:: handleSyncConfig(AsyncUDPPacket *packet, unsigned int readOff)
{
	unsigned long id;
	memcpy(&id, &packet->data()[readOff], sizeof(id));
	readOff += sizeof(id);

	VirtualDevice* device = LEDDeviceManager.getDevice(id);

	if (device) {
		double posStart;
		memcpy(&posStart, &packet->data()[readOff], sizeof(posStart));
		readOff += sizeof(posStart);

		double posEnd;
		memcpy(&posEnd, &packet->data()[readOff], sizeof(posEnd));
		readOff += sizeof(posEnd);

		double timeVal;
		memcpy(&timeVal, &packet->data()[readOff], sizeof(timeVal));
		readOff += sizeof(timeVal);


		device->setPosStart(posStart);
		device->setPosEnd(posEnd);
		device->setTimeValue(timeVal);
	}
}

void SyncManager::handleEffectIndex(AsyncUDPPacket *packet, unsigned int readOff)
{
	unsigned long id;
	memcpy(&id, &packet->data()[readOff], sizeof(id));
	readOff += sizeof(id);

	VirtualDevice* device = LEDDeviceManager.getDevice(id);

	if (device) {
		int index;
		memcpy(&index, &packet->data()[readOff], sizeof(index));
		readOff += sizeof(index);

		device->setEffect(index);
	}
}

void SyncManager::handleEffectData(AsyncUDPPacket *packet, unsigned int readOff)
{
	unsigned long id;
	memcpy(&id, &packet->data()[readOff], sizeof(id));
	readOff += sizeof(id);

	VirtualDevice* device = LEDDeviceManager.getDevice(id);

	if (device) {
		unsigned char *data = &packet->data()[readOff];
		unsigned int length = packet->length() - readOff;

		device->setEffect(data, length);
	}
}

void SyncManager::startSync(VirtualDevice *device, IPAddress ip, unsigned long id)
{
	std::vector<SyncDevice*> *syncedList = NULL;

	std::map<VirtualDevice*, std::vector<SyncDevice*>*>::iterator it = _syncs.find(device);
	if (it != _syncs.end()) {
		syncedList = it->second;
	} else {
		syncedList = new std::vector<SyncDevice*>();
		_syncs[device] = syncedList;
	}

	// check if we are already synced
	for (std::vector<SyncDevice*>::iterator it = syncedList->begin(); it != syncedList->end(); it++) {
		if ((*it)->getIp() == ip && (*it)->getId() == id) {
			return; // this device is already getting synced
		}
	}


	SyncDevice *syncDevice = NULL;

	if (ip == WiFi.localIP()) {
		syncDevice = new InternalSync(id);
	} else {
		syncDevice = new ExternalSync(ip, id, &_udp);
	}
	// TODO: search for this device in the synced list and reset it

	syncedList->push_back(syncDevice);

	deviceChanged(device);
}

void SyncManager::deviceChanged(VirtualDevice *device)
{
	std::map<VirtualDevice*, std::vector<SyncDevice*>*>::iterator it = _syncs.find(device);
	if (it != _syncs.end()) {
		VirtualDevice *master = it->first;
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

	double timeValue = master->getLastTimeValue();

	for (std::vector<SyncDevice*>::iterator it = slaves->begin(); it != slaves->end(); it++) {
		posStart = posEnd;

		endCount += (*it)->getLedCount();
		posEnd = (double) endCount / totalCount;

		(*it)->doSync(posStart, posEnd, timeValue);
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

		for (std::vector<SyncDevice*>::iterator it2 = it->second->begin(); it2 != it->second->end(); it2++) {
			SyncDevice* syncDevice = *it2;

			if (syncDevice->needsSync()) {
				if (syncDevice->getLedCount() > 0) {
					// TODO: sync!
					doSync(master, it->second);
				} else {
					syncDevice->retrieveLedCount();
				}
			}
		}
	}

	if (currentMillis - _lastDeviceSearchMillis > 5000.0) { // search every 5 seconds
		searchSyncableDevices();
		_lastDeviceSearchMillis = currentMillis;
	}
}

SyncManager LEDSyncManager;
