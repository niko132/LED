#include "PhysicalDevice.h"

PhysicalDevice::PhysicalDevice(int ledCount) :
    _strip(ledCount)
{
    _ledCount = ledCount;
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
    // _strip.Show();
}

int PhysicalDevice::getLedCount()
{
    return _ledCount;
}

unsigned char* PhysicalDevice::getPixelBuf()
{
    return _pixelBuf;
}