#ifndef SYNCMANAGER_H
#define SYNCMANAGER_H

#include "VirtualDevice.h"
#include "DeviceManager.h"
#include "Magic.h"

#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncUDP.h>

#include <map>
#include <vector>

#include "ESPLogger.h"

class SyncDevice {
	private:
		IPAddress _ip;
		unsigned long _id = 0;

		int _ledCount = 0;

		bool _ledCountRequested = false;
		bool _needsSync = true;

	protected:
		virtual void retrieveLedCountInt() = 0;
		virtual void doSyncInt(double posStart, double posEnd, unsigned long timeOffset) = 0;

	public:
		SyncDevice(IPAddress ip, unsigned long id) {
			_ip = ip;
			_id = id;
		};

		IPAddress getIp() {
			return _ip;
		};

		unsigned long getId() {
			return _id;
		};

		int getLedCount() {
			return _ledCount;
		};

		bool needsSync() {
			return _needsSync;
		};

		void setLedCount(int count) {
			_ledCount = count;
			_needsSync = true;
		}

		void retrieveLedCount() {
			if (!_ledCountRequested) {
				retrieveLedCountInt();
				_ledCountRequested = true;
			}
		};

		void doSync(double posStart, double posEnd, unsigned long timeOffset) {
			doSyncInt(posStart, posEnd, timeOffset);
			_needsSync = false;
		};

		virtual void setSyncConfig(IPAddress ip, unsigned long id, int mode) = 0;

		virtual void setEffect(String name) = 0;
		virtual void setEffect(unsigned char *data, unsigned int length) = 0;
};

class InternalSync : public SyncDevice {
	private:
		VirtualDevice *_device = NULL;

	protected:
		void retrieveLedCountInt() {
			if (_device) {
				setLedCount(_device->getLedCount());
			}
		};

		void doSyncInt(double posStart, double posEnd, unsigned long timeOffset) {
			_device->setPosStart(posStart);
			_device->setPosEnd(posEnd);

			_device->syncTimeOffset(timeOffset);
		};

	public:
		InternalSync(unsigned long id) : SyncDevice(WiFi.localIP(), id) {
			_device = LEDDeviceManager.getDevice(id);
		};

		void setSyncConfig(IPAddress ip, unsigned long id, int mode) {
			_device->setSyncMode(mode);
		};

		void setEffect(String name) {
			_device->syncEffect(name);
		};

		void setEffect(unsigned char *data, unsigned int length) {
			_device->setEffect(data, length);
		};
};

class ExternalSync : public SyncDevice {
	private:
		AsyncUDP *_udp = NULL;

	protected:
		void retrieveLedCountInt() {
			unsigned char type = 11; // request led count
			unsigned long id = getId();

			MagicWriter writer;
			writer.write(type);
			writer.write(id);

			unsigned char *buf = NULL;
			unsigned int length = 0;
			buf = writer.getData(&length);

			Logger.println("Sending led count request...");

			_udp->writeTo(buf, length, getIp(), 6789);
		};

		void doSyncInt(double posStart, double posEnd, unsigned long timeOffset) {
			unsigned char type = 13; // response sync config
			unsigned long id = getId();

			MagicWriter writer;
			writer.write(type);
			writer.write(id);
			writer.write(posStart);
			writer.write(posEnd);
			writer.write(timeOffset);

			unsigned char *buf = NULL;
			unsigned int length = 0;
			buf = writer.getData(&length);

			Logger.println("Sending sync data...");

			_udp->writeTo(buf, length, getIp(), 6789);
		};

	public:
		ExternalSync(IPAddress ip, unsigned long id, AsyncUDP *udp) : SyncDevice(ip, id) {
			_udp = udp;
		};

		void setSyncConfig(IPAddress ip, unsigned long id, int mode) {
			unsigned char type = 16; // response sync Config
			unsigned long ownId = getId();

			MagicWriter writer;
			writer.write(type);
			writer.write(ownId);
			writer.write(mode);

			unsigned char *buf = NULL;
			unsigned int length = 0;
			buf = writer.getData(&length);

			Logger.println("Sending Config...");

			_udp->writeTo(buf, length, getIp(), 6789);
		};

		void setEffect(String name) {
			unsigned char type = 14; // response effect name
			unsigned long id = getId();

			MagicWriter writer;
			writer.write(type);
			writer.write(id);
			writer.write(name);

			unsigned char *buf = NULL;
			unsigned int length = 0;
			buf = writer.getData(&length);

			Logger.println("Sending effect...");

			_udp->writeTo(buf, length, getIp(), 6789);
		};

		void setEffect(unsigned char *data, unsigned int length) {
			unsigned char type = 15; // response effect data
			unsigned long id = getId();

			MagicWriter writer;
			writer.write(type);
			writer.write(id);
			writer.write(data, length);

			unsigned char *buf = NULL;
			unsigned int bufLength = 0;
			buf = writer.getData(&bufLength);

			Logger.println("Sending effect data...");

			_udp->writeTo(buf, bufLength, getIp(), 6789);
		};
};

class SyncManager {
	private:
		AsyncWebServer *_server;
		AsyncUDP _udp;

		unsigned long _lastDeviceSearchMillis = 0;

		std::map<String, std::vector<unsigned long>*> _syncableDevices;
		std::map<VirtualDevice*, std::vector<SyncDevice*>*> _syncs;

	public:
		SyncManager();
		void begin(AsyncWebServer *server);

		void handleSearchRequest(MagicReader *reader, AsyncUDPPacket *packet);
		void handleSearchResponse(MagicReader *reader, AsyncUDPPacket *packet);
		void handleLedCountRequest(MagicReader *reader, AsyncUDPPacket *packet);
		void handleLedCountResponse(MagicReader *reader, AsyncUDPPacket *packet);
		void handleSyncData(MagicReader *reader, AsyncUDPPacket *packet);
		void handleEffectName(MagicReader *reader, AsyncUDPPacket *packet);
		void handleEffectData(MagicReader *reader, AsyncUDPPacket *packet);
		void handleSyncConfig(MagicReader *reader, AsyncUDPPacket *packet);
		void handlePixelData(MagicReader *reader, AsyncUDPPacket *packet);

		void startSync(VirtualDevice *device, IPAddress ip, unsigned long id, int mode);
		void deviceChanged(VirtualDevice *device);

		void doSync(VirtualDevice *master, std::vector<SyncDevice*> *slaves);
		void searchSyncableDevices();

		void update();
};

extern SyncManager LEDSyncManager;

#endif // SYNCMANAGER_H
