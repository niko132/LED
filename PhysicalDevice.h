#ifndef PHYSICALDEVICE_H
#define PHYSICALDEVICE_H

#include <NeoPixelBus.h>

class PhysicalDevice {
    protected:
        unsigned int _ledCount;
		bool _mirror;
        NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> _strip;
        unsigned char* _pixelBuf;

    public:
        PhysicalDevice(unsigned int ledCount);
        ~PhysicalDevice();

        void begin();
        unsigned int getLedCount();
        unsigned char* getPixelBuf();
        
        void clear();
        virtual void update();
};

#endif // PHYSICALDEVICE_H