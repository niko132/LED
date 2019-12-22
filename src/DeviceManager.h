#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include "PhysicalDevice.h"
#include "VirtualDevice.h"

#include <vector>

class DeviceManager {
    private:
        PhysicalDevice *_device;
        std::vector<VirtualDevice*> _virtualDevices;

    public:
        DeviceManager(PhysicalDevice *device);
        DeviceManager(int ledCount);
        ~DeviceManager();

        void begin();

        int addDevice(int offset, int ledCount);
        bool editDevice(int id, int offset, int ledCount);
        bool removeDevice(int id);
};

#endif // DEVICEMANAGER_H