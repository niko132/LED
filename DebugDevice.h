#ifndef DEBUGDEVICE_H
#define DEBUGDEVICE_H

#include "PhysicalDevice.h"

#include <ESPAsyncUDP.h>

class DebugDevice : public PhysicalDevice {
	private:
		AsyncUDP _udp;
	
	public:
		DebugDevice(int ledCount) : PhysicalDevice(ledCount)
		{
			
		};
		
		void update()
		{
			unsigned char buf[4 * _ledCount];
			
			for (int i = 0; i < _ledCount; i++) {
				int index = i;
		
				if (_mirror)
					index = _ledCount - i - 1;
				
				buf[index * 4] = (unsigned char) index;
				buf[index * 4 + 1] = _pixelBuf[i * 3];
				buf[index * 4 + 2] = _pixelBuf[i * 3 + 1];
				buf[index * 4 + 3] = _pixelBuf[i * 3 + 2];
			}

			IPAddress addr(192, 168, 2, 16);
			_udp.writeTo(buf, 4 * _ledCount, addr, 9876);
		};
};

#endif // DEBUGDEVICE_H