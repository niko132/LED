#ifndef PHYSICALDEVICE_H
#define PHYSICALDEVICE_H

#include <NeoPixelBus.h>

class PhysicalDevice {
    private:
        int _ledCount;
        NeoPixelBus<NeoRgbFeature, NeoEsp8266Dma800KbpsMethod> _strip;
        unsigned char* _pixelBuf;

    public:
        PhysicalDevice(int ledCount);
        ~PhysicalDevice();

        void begin();
        int getLedCount();
        unsigned char* getPixelBuf();
};

#endif // PHYSICALDEVICE_H