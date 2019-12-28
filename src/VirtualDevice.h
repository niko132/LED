#ifndef VIRTUALDEVICE_H
#define VIRTUALDEVICE_H

#include "PhysicalDevice.h"

class VirtualDevice {
    private:
        PhysicalDevice *_device;
        int _startIndex;
        int _endIndex;
        int _id;

        std::vector<int*> _subIndices;

    public:
        VirtualDevice(PhysicalDevice *device, int startIndex, int endIndex);

        void begin();

        void setStartIndex(int startIndex);
        void setEnddIndex(int endIndex);

        void resetAreas();
        void removeArea(int startIndex, int endIndex);

        int getStartIndex();
        int getEndIndex();
        int getLedCount();
        int getId();
};

#endif // VIRTUALDEVICE_H