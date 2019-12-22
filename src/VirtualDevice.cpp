#include "VirtualDevice.h"

VirtualDevice::VirtualDevice(PhysicalDevice *device, int id, int offset, int ledCount)
{
    _device = device;
    _pixelBuf = &device->getPixelBuf()[offset * 3];
    _id = id;
    _ledCount = ledCount;
}

void VirtualDevice::begin()
{
    
}

void VirtualDevice::setId(int id)
{
    _id = id;
    // TODO: implement web functions
}

void VirtualDevice::setOffset(int offset)
{
    _pixelBuf = &_device->getPixelBuf()[offset * 3];
}

void VirtualDevice::setLedCount(int ledCount)
{
    _ledCount = ledCount;
}