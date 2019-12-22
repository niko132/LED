#include "DeviceManager.h"

DeviceManager::DeviceManager(PhysicalDevice *device)
{
    _device = device;

    _virtualDevices.push_back(new VirtualDevice(_device, 0, 0, _device->getLedCount()));
}

DeviceManager::DeviceManager(int ledCount)
{
    _device = new PhysicalDevice(ledCount);

    _virtualDevices.push_back(new VirtualDevice(_device, 0, 0, ledCount)); // add a virtual device for the whole strip
}

DeviceManager::~DeviceManager()
{
    if (_device) {
        delete _device;
        _device = NULL;
    }

    for (int i = 0; i < _virtualDevices.size(); i--) {
        delete _virtualDevices[i];
    }

    _virtualDevices.clear();
}

void DeviceManager::begin()
{
    if (_device)
        _device->begin();

    for (int i = 0; i < _virtualDevices.size(); i++) {
        _virtualDevices[i]->begin();
    }
}

int DeviceManager::addDevice(int offset, int ledCount)
{
    int id = _virtualDevices.size(); // id is equal to the index
    _virtualDevices.push_back(new VirtualDevice(_device, id, offset, ledCount));
}

bool DeviceManager::editDevice(int id, int offset, int ledCount)
{
    if (id < _virtualDevices.size()) {
        _virtualDevices[id]->setOffset(offset);
        _virtualDevices[id]->setLedCount(ledCount);

        return true;
    }

    return false;
}

bool DeviceManager::removeDevice(int id)
{
    if (id < _virtualDevices.size()) {
        delete _virtualDevices[id];
        _virtualDevices.erase(_virtualDevices.begin() + id);

        for (int i = id; i < _virtualDevices.size(); i++) {
            _virtualDevices[i]->setId(i);
        }

        return true;
    }

    return false;
}