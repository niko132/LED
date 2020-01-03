#include "VirtualDevice.h"
#include "ColorCycle.h"

#include <FastLED.h>

#include <stdio.h>
#include <iostream>

VirtualDevice::VirtualDevice(PhysicalDevice *device, int startIndex, int endIndex, int mode)
{
    _device = device;
    _startIndex = startIndex;
    _endIndex = endIndex;
    _mode = mode;

    //TODO: update id generation
    _id = ESP.getCycleCount();

    // default effect
    _effect = new ColorCycle();

    resetAreas();
}

void VirtualDevice::begin()
{
    // TODO: maybe move to constructor
    
}

void VirtualDevice::setStartIndex(int startIndex)
{
    _startIndex = startIndex;
}

void VirtualDevice::setEnddIndex(int endIndex)
{
    _endIndex = endIndex;
}

void VirtualDevice::setMode(int mode)
{
    _mode = mode;
}

void VirtualDevice::resetAreas()
{
    _subIndices.clear();
    _subIndices.push_back(new int[2] {_startIndex, _endIndex});
}

void VirtualDevice::removeArea(int startIndex, int endIndex)
{
    for (int i = _subIndices.size() - 1; i >= 0; i--) {
        if (startIndex <= _subIndices[i][0] && endIndex >= _subIndices[i][0] && endIndex < _subIndices[i][1]) {
            _subIndices[i][0] = endIndex;
        } else if (endIndex >= _subIndices[i][1] && startIndex <= _subIndices[i][1] && startIndex > _subIndices[i][0]) {
            _subIndices[i][1] = startIndex;
        } else if (startIndex > _subIndices[i][0] && endIndex < _subIndices[i][1]) {
            int *a = new int[2] {_subIndices[i][0], startIndex};
            int *b = new int[2] {endIndex, _subIndices[i][1]};

            _subIndices.erase(_subIndices.begin() + i);
            _subIndices.insert(_subIndices.begin() + i, a);
            _subIndices.insert(_subIndices.begin() + 1, b);
        } else if (startIndex <= _subIndices[i][0] && endIndex >= _subIndices[i][1]) {
            _subIndices.erase(_subIndices.begin() + i);
        }
    }



    /*
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
    */
}

int VirtualDevice::getStartIndex()
{
    return _startIndex;
}

int VirtualDevice::getEndIndex()
{
    return _endIndex;
}

int VirtualDevice::getLedCount()
{
    // TODO: implement LedCount with subindices
    return _endIndex - _startIndex;
}

int VirtualDevice::getMode()
{
    return _mode;
}

unsigned long VirtualDevice::getId()
{
    return _id;
}

void VirtualDevice::update()
{
    double timeValue = 0.0;

    // TODO: update time value

    // set everything to black
    _device->clear();

    for (int i = 0; i < _subIndices.size(); i++) {
        for (int index = _subIndices[i][0]; index < _subIndices[i][1]; index++) {
            double posValue = 0.0;

            if (_mode == 0) {
                posValue = (double) (index - _startIndex) / (double) getLedCount();
            }

            // TODO: implement effect updating
            // effect.update(timeValue, posValue, &_device->getPixelBuf()[index * 3]);

            CRGB color = _effect->update(timeValue, posValue);

            // post process color...

            unsigned char* p = &_device->getPixelBuf()[index * 3];
            p[0] = color.r;
            p[1] = color.g;
            p[2] = color.b;
        }
    }

    _device->update();
}

void VirtualDevice::debug()
{
    for (int i = _startIndex; i < _endIndex; i++) {
        bool found = false;

        for (int j = 0; j < _subIndices.size(); j++) {
            if (i >= _subIndices[j][0] && i < _subIndices[j][1]) {
                found = true;
                break;
            }
        }

        if (found) {
            std::cout << i << " ";
        } else {
            std::cout << "*" << " ";
        }
    }
}