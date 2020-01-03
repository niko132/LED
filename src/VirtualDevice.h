#ifndef VIRTUALDEVICE_H
#define VIRTUALDEVICE_H

#include "PhysicalDevice.h"
#include "Effect.h"

#include <vector>

class VirtualDevice {
    private:
        PhysicalDevice *_device;
        int _startIndex;
        int _endIndex;
        int _mode;
        unsigned long _id;

        std::vector<int*> _subIndices;

        Effect *_effect;

    public:
        VirtualDevice(PhysicalDevice *device, int startIndex, int endIndex, int mode);

        void begin();

        void setStartIndex(int startIndex);
        void setEnddIndex(int endIndex);
        void setMode(int mode);

        void resetAreas();
        void removeArea(int startIndex, int endIndex);

        int getStartIndex();
        int getEndIndex();
        int getLedCount();
        int getMode();
        unsigned long getId();

        void update();

        void debug();
};

#endif // VIRTUALDEVICE_H