#include "VirtualDevice.h"

VirtualDevice::VirtualDevice(PhysicalDevice *device, int offset, int ledCount)
{
    _device = device;
    _pixelBuf = &device->getPixelBuf()[offset * 3];
    _ledCount = ledCount;
}

void VirtualDevice::begin()
{
    
}

void VirtualDevice::setOffset(int offset)
{
    _offset = offset;
}

void VirtualDevice::setLedCount(int ledCount)
{
    _ledCount = ledCount;
}

void VirtualDevice::removeArea(int startIndex, int length)
{
    std::vector<int*> indices;

    indices.push_back(new int[2] {_offset, _offset + _ledCount});

    for (int i = indices.size() - 1; i >= 0; i--) {
        if (startIndex >= indices[i][0] && startIndex <= indices[i][1] && startIndex + length > indices[i][1]) {
            indices[i][1] = startIndex - 1;
        } else if (startIndex + length >= indices[i][0] && startIndex + length <= indices[i][1] && startIndex < indices[i][0]) {
            indices[i][0] = startIndex + length;
        } else {
            int *a = new int[2] {indices[i][0], startIndex};
            int *b = new int[2] {startIndex + length, indices[i][1]};

            indices.erase(indices.begin() + i);
            indices.push_back(a);
            indices.push_back(b);
        }
    }
}