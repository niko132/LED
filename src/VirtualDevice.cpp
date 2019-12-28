#include "VirtualDevice.h"

VirtualDevice::VirtualDevice(PhysicalDevice *device, int startIndex, int endIndex)
{
    _device = device;
    _startIndex = startIndex;
    endIndex = endIndex;

    //TODO generate random ID
}

void VirtualDevice::begin()
{
    
}

void VirtualDevice::setStartIndex(int startIndex)
{
    _startIndex = startIndex;
}

void VirtualDevice::setEnddIndex(int endIndex)
{
    _endIndex = endIndex;
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
            _subIndices[i][0] = endIndex + 1;
        } else if (endIndex >= _subIndices[i][1] && startIndex <= _subIndices[i][1] && startIndex > _subIndices[i][0]) {
            _subIndices[i][1] = startIndex - 1;
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

int VirtualDevice::getId()
{
    return _id;
}