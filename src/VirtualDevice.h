#ifndef VIRTUALDEVICE_H
#define VIRTUALDEVICE_H

#include "PhysicalDevice.h"

class VirtualDevice {
    private:
        PhysicalDevice *_device;
        int _offset;
        int _ledCount;

        std::vector<VirtualDevice*> _subdevices;

    public:
        VirtualDevice(PhysicalDevice *device, int offset, int ledCount);

        void begin();

        void setOffset(int offset);
        void setLedCount(int ledCount);

        void removeArea(int startIndex, int length);
};

#endif // VIRTUALDEVICE_H