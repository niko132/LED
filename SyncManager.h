#ifndef SYNCMANAGER_H
#define SYNCMANAGER_H

#include "VirtualDevice.h"
#include "DeviceManager.h"

#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncUDP.h>

#include <map>
#include <vector>

const unsigned char MAGIC[4] = { 0x92, 0xB3, 0x1F, 0x12 };
const unsigned char MAGIC_LENGTH = 4;

class SyncDevice {
	private:
		IPAddress _ip;
		unsigned long _id = 0;
		
		int _ledCount = 0;
		
		bool _ledCountRequested = false;
		bool _needsSync = true;
		
	protected:		
		virtual void retrieveLedCountInt() = 0;
		virtual void doSyncInt(double posStart, double posEnd, double timeValue) = 0;
	
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
		
		void doSync(double posStart, double posEnd, double timeValue) {
			doSyncInt(posStart, posEnd, timeValue);
			_needsSync = false;
		};
		
		virtual void setEffect(int index) = 0;
		virtual void setEffect(unsigned char *data, unsigned int length) = 0;
};

class InternalSync : public SyncDevice {
	private:
		VirtualDevice *_device = NULL;
		
	protected:
		void retrieveLedCountInt() {
			if (_device) {
				setLedCount(_device->getCount());
			}
		};
		
		void doSyncInt(double posStart, double posEnd, double timeValue) {
			_device->setPosStart(posStart);
			_device->setPosEnd(posEnd);
			_device->setTimeValue(timeValue);
		};
	
	public:
		InternalSync(unsigned long id) : SyncDevice(WiFi.localIP(), id) {
			_device = LEDDeviceManager.getDevice(id);
		};
		
		void setEffect(int index) {
			_device->setEffect(index);
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
			unsigned char *buf = new unsigned char[MAGIC_LENGTH + 1 + sizeof(unsigned long)];
			unsigned int writeOff = 0;
			
			memcpy(&buf[writeOff], MAGIC, MAGIC_LENGTH);
			writeOff += MAGIC_LENGTH;
		
			buf[writeOff++] = 11; // request led count
			
			unsigned long id = getId();
		
			memcpy(&buf[writeOff], &id, sizeof(id));
			writeOff += sizeof(id);
			
			_udp->writeTo(buf, writeOff, getIp(), 6789);
			
			delete[] buf;
		};
		
		void doSyncInt(double posStart, double posEnd, double timeValue) {
			unsigned char *buf = new unsigned char[MAGIC_LENGTH + 1 + sizeof(unsigned long) + 3 * sizeof(double)];
			unsigned int writeOff = 0;
			
			memcpy(&buf[writeOff], MAGIC, MAGIC_LENGTH);
			writeOff += MAGIC_LENGTH;
		
			buf[writeOff++] = 13; // response sync config
		
			unsigned long id = getId();
		
			memcpy(&buf[writeOff], &id, sizeof(id));
			writeOff += sizeof(id);
		
			memcpy(&buf[writeOff], &posStart, sizeof(posStart));
			writeOff += sizeof(posStart);
		
			memcpy(&buf[writeOff], &posEnd, sizeof(posEnd));
			writeOff += sizeof(posEnd);
		
			memcpy(&buf[writeOff], &timeValue, sizeof(timeValue));
			writeOff += sizeof(timeValue);
		
			_udp->writeTo(buf, writeOff, getIp(), 6789);
		
			delete[] buf;
		};
	
	public:
		ExternalSync(IPAddress ip, unsigned long id, AsyncUDP *udp) : SyncDevice(ip, id) {
			_udp = udp;
		};
		
		void setEffect(int index) {
			unsigned char *buf = new unsigned char[MAGIC_LENGTH + 1 + sizeof(unsigned long) + sizeof(int)];
			unsigned int writeOff = 0;
			
			memcpy(&buf[writeOff], MAGIC, MAGIC_LENGTH);
			writeOff += MAGIC_LENGTH;
		
			buf[writeOff++] = 14; // response effect index
		
			unsigned long id = getId();
		
			memcpy(&buf[writeOff], &id, sizeof(id));
			writeOff += sizeof(id);
			
			memcpy(&buf[writeOff], &index, sizeof(index));
			writeOff += sizeof(index);
			
			_udp->writeTo(buf, writeOff, getIp(), 6789);
		};
		
		void setEffect(unsigned char *data, unsigned int length) {
			unsigned char *buf = new unsigned char[MAGIC_LENGTH + 1 + sizeof(unsigned long) + length];
			unsigned int writeOff = 0;
			
			memcpy(&buf[writeOff], MAGIC, MAGIC_LENGTH);
			writeOff += MAGIC_LENGTH;
		
			buf[writeOff++] = 15; // response effect data
		
			unsigned long id = getId();
		
			memcpy(&buf[writeOff], &id, sizeof(id));
			writeOff += sizeof(id);
			
			memcpy(&buf[writeOff], data, length);
			writeOff += length;
		};
};

class SyncManager {
	private:
		typedef struct
		{
			VirtualDevice *master;
			IPAddress ip;
			unsigned long id;
		} SyncRequest;
		
		typedef struct
		{
			IPAddress ip;
			unsigned long id;
			int ledCount;
		} SyncedDevice;
		
		AsyncWebServer *_server;
		AsyncUDP _udp;
		
		unsigned long _lastDeviceSearchMillis = 0;
		
		std::map<String, std::vector<unsigned long>*> _syncableDevices;
		std::map<VirtualDevice*, std::vector<SyncDevice*>*> _syncs;
		
	public:
		SyncManager();
		void begin(AsyncWebServer *server);
		
		void handleSearchRequest(AsyncUDPPacket *packet, unsigned int readOff);
		void handleSearchResponse(AsyncUDPPacket *packet, unsigned int readOff);
		void handleLedCountRequest(AsyncUDPPacket *packet, unsigned int readOff);
		void handleLedCountResponse(AsyncUDPPacket *packet, unsigned int readOff);
		void handleSyncConfig(AsyncUDPPacket *packet, unsigned int readOff);
		void handleEffectIndex(AsyncUDPPacket *packet, unsigned int readOff);
		void handleEffectData(AsyncUDPPacket *packet, unsigned int readOff);
		
		void startSync(VirtualDevice *device, IPAddress ip, unsigned long id);
		void deviceChanged(VirtualDevice *device);
		
		void doSync(VirtualDevice *master, std::vector<SyncDevice*> *slaves);
		void searchSyncableDevices();
	
		void update();
};

extern SyncManager LEDSyncManager;

#endif // SYNCMANAGER_H