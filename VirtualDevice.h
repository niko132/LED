#ifndef VIRTUALDEVICE_H
#define VIRTUALDEVICE_H

#include "PhysicalDevice.h"
#include "Palette.h"
#include "Effect.h"

#include "CoverAlgorithm.h"
#include "SyncAlgorithm.h"

#include <vector>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>

class VirtualDevice {
    private:
        PhysicalDevice *_device;
        int _mode;
        unsigned long _id;

        CoverAlgorithm *_coverAlgorithm = NULL;
        SyncAlgorithm *_syncAlgorithm = NULL;

		double _posStart = 0.0;
		double _posEnd = 1.0;

		AsyncWebServer *_server;

		Palette *_palette = NULL;

		AsyncCallbackJsonWebHandler *_effectHandler = NULL;
		AsyncCallbackJsonWebHandler *_posHandler = NULL;
		AsyncCallbackJsonWebHandler *_syncHandler = NULL;

    public:
        VirtualDevice(PhysicalDevice *device, unsigned int startIndex, unsigned int endIndex, int mode);
		VirtualDevice(PhysicalDevice *device, unsigned long id);
		~VirtualDevice();

        void begin(AsyncWebServer *server);

        void setStartIndex(int startIndex);
        void setEndIndex(int endIndex);
        void setMode(int mode);

		void setEffect(String name);
		void setEffect(unsigned char *buf, unsigned int length);
		void setTimeOffset(unsigned long timeOffset);

		void setPosStart(double posStart);
		void setPosEnd(double posEnd);

        void resetCovered();
        void addCovered(unsigned int startIndex, unsigned int endIndex);

        unsigned int getStartIndex();
        unsigned int getEndIndex();
		unsigned int getLedCount();
        int getMode();
		double getPosStart();
		double getPosEnd();
        unsigned long getId();

		unsigned long getTimeOffset();
		Effect *getEffect();

		void serialize();

        void update();
};

#endif // VIRTUALDEVICE_H
