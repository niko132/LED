#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include "PhysicalDevice.h"
#include "VirtualDevice.h"

#include <vector>

class DeviceManager {
    private:
        PhysicalDevice *_device;
        std::vector<int*> _deviceConfigs;
        std::vector<VirtualDevice*> _virtualDevices;

        std::vector<std::vector<VirtualDevice*>*> _deviceHierarchy;

    public:
        DeviceManager(PhysicalDevice *device);
        DeviceManager(int ledCount);
        ~DeviceManager();

        void begin();

        int addDevice(int startIndex, int endIndex, int zIndex, int mode);
        void addDevice(VirtualDevice *newDevice, int zIndex);
        bool editDevice(int id, int startIndex, int endIndex, int zIndex, int mode);
        void editDevice(VirtualDevice *device, int startIndex, int endIndex, int zIndex, int mode);
        bool removeDevice(int id);
        bool removeDevice(VirtualDevice *device);

        void buildDevices();

        void update();

        void debug();
};

#endif // DEVICEMANAGER_H