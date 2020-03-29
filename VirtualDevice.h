#ifndef VIRTUALDEVICE_H
#define VIRTUALDEVICE_H

#include "PhysicalDevice.h"
#include "Palette.h"
#include "Effect.h"

#include <vector>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>

class VirtualDevice {
    private:
		typedef struct
		{
			IPAddress ip;
			unsigned long id;
		} SyncRequest;
		
		typedef struct
		{
			IPAddress ip;
			unsigned long id;
			int ledCount;
		} SyncedDevice;
	
        PhysicalDevice *_device;
        int _startIndex;
        int _endIndex;
        int _mode;
        unsigned long _id;
		
		double _posStart = 0.0;
		double _posEnd = 1.0;

        std::vector<int*> _subIndices;
		
		std::vector<SyncRequest> _syncRequests;
		std::vector<SyncedDevice> _syncedDevices;
		AsyncWebServer *_server;

		Palette *_palette = NULL;
        Effect *_effect = NULL;
		int _effectIndex = 0;
		double _lastTimeValue = 0;
		
		AsyncCallbackJsonWebHandler *_effectHandler = NULL;
		AsyncCallbackJsonWebHandler *_posHandler = NULL;
		AsyncCallbackJsonWebHandler *_syncHandler = NULL;

    public:
        VirtualDevice(PhysicalDevice *device, int startIndex, int endIndex, int mode);
		~VirtualDevice();

        void begin(AsyncWebServer *server);

        void setStartIndex(int startIndex);
        void setEndIndex(int endIndex);
        void setMode(int mode);
		
		void setEffect(int index);
		void setEffect(unsigned char *buf, unsigned int length);
		void setTimeValue(double val);
		
		void setPosStart(double posStart);
		void setPosEnd(double posEnd);

        void resetAreas();
        void removeArea(int startIndex, int endIndex);

        int getStartIndex();
        int getEndIndex();
		int getCount();
        int getLedCount();
		int getLedRangeCount();
        int getMode();
		int getEffectIndex();
		double getPosStart();
		double getPosEnd();
        unsigned long getId();
		
		double getLastTimeValue();
		Effect *getEffect();

        void update(unsigned long delta);
};

#endif // VIRTUALDEVICE_H