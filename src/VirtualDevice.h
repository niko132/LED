#ifndef VIRTUALDEVICE_H
#define VIRTUALDEVICE_H

#include "PhysicalDevice.h"

class VirtualDevice {
    private:
        PhysicalDevice *_device;
        unsigned char *_pixelBuf;
        int _id;
        int _ledCount;

    public:
        VirtualDevice(PhysicalDevice *device, int id, int offset, int ledCount);

        void begin();

        void setId(int id);
        void setOffset(int offset);
        void setLedCount(int ledCount);
};

#endif // VIRTUALDEVICE_H