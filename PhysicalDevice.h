#ifndef PHYSICALDEVICE_H
#define PHYSICALDEVICE_H

#include <NeoPixelBus.h>

class PhysicalDevice {
    private:
        int _ledCount;
		bool _mirror;
        NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> _strip;
        unsigned char* _pixelBuf;

    public:
        PhysicalDevice(int ledCount);
        ~PhysicalDevice();

        void begin();
        int getLedCount();
        unsigned char* getPixelBuf();
        
        void clear();
        void update();
};

#endif // PHYSICALDEVICE_H