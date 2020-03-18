#include "PhysicalDevice.h"

#include <stdio.h>

PhysicalDevice::PhysicalDevice(int ledCount) :
    _strip(ledCount)
{
    _ledCount = ledCount;
	_mirror = true;	
    _pixelBuf = new unsigned char[ledCount * 3]; // 3 colors per led
}

PhysicalDevice::~PhysicalDevice()
{
    delete[] _pixelBuf;
    _pixelBuf = NULL;
}

void PhysicalDevice::begin()
{
    _strip.Begin();
    // TODO: set all pixels to black
    _strip.Show();
}

int PhysicalDevice::getLedCount()
{
    return _ledCount;
}

unsigned char* PhysicalDevice::getPixelBuf()
{
    return _pixelBuf;
}

void PhysicalDevice::clear()
{
    memset(_pixelBuf, 0, _ledCount * 3);
}

void PhysicalDevice::update()
{
    for (int i = 0; i < _ledCount; i++) {
		int index = i;
		
		if (_mirror)
			index = _ledCount - i - 1;
		
        _strip.SetPixelColor(index, RgbColor(_pixelBuf[i * 3], _pixelBuf[i * 3 + 1], _pixelBuf[i * 3 + 2]));
    }

    _strip.Show();
}