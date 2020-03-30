#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include "PhysicalDevice.h"
#include "DebugDevice.h"
#include "VirtualDevice.h"

#include <vector>
#include <map>
#include <ESPAsyncWebServer.h>

class DeviceManager {
    private:
        PhysicalDevice *_device = NULL;
		AsyncWebServer *_server = NULL;

        std::vector<std::vector<VirtualDevice*>*> _deviceHierarchy;
		
		unsigned long _lastUpdateMillis = 0;
		bool _onState = true;
		
		void begin(AsyncWebServer *server);
		
		bool loadConfig(unsigned int ledCount);
		void saveConfig();

    public:
        ~DeviceManager();

        void begin(DebugDevice *device, AsyncWebServer *server);
        void begin(unsigned int ledCount, AsyncWebServer *server);
		
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
		
        void update();
};

extern DeviceManager LEDDeviceManager;

#endif // DEVICEMANAGER_H