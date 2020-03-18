#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include "PhysicalDevice.h"
#include "VirtualDevice.h"

#include <vector>
#include <map>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncUDP.h>

class DeviceManager {
    private:
        PhysicalDevice *_device;
		AsyncWebServer *_server = NULL;

        std::vector<std::vector<VirtualDevice*>*> _deviceHierarchy;
		
		std::map<String, std::vector<unsigned long>> _syncableDevices;
		
		unsigned long _lastUpdateMillis = 0;
		unsigned long _lastDeviceSearchMillis = 0;
		bool _onState = true;
		
		AsyncUDP _udp;

    public:
        DeviceManager(PhysicalDevice *device);
        DeviceManager(int ledCount);
        ~DeviceManager();

        void begin(AsyncWebServer *server);
		
		VirtualDevice* getDevice(unsigned long id);
		VirtualDevice* getDeviceAt(int index);
		int getDeviceZIndex(VirtualDevice *device);
		int getDeviceCount();

        unsigned long addDevice(int startIndex, int endIndex, int zIndex, int mode);
        void addDevice(VirtualDevice *newDevice, int zIndex);
        bool editDevice(int id, int startIndex, int endIndex, int zIndex, int mode);
        bool editDevice(VirtualDevice *device, int startIndex, int endIndex, int zIndex, int mode);
        VirtualDevice* removeDevice(int id);
        bool removeDevice(VirtualDevice *device);

        void buildDevices();
		
		void searchRemoteDevices();

        void update();
};

#endif // DEVICEMANAGER_H