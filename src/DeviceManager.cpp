#include "DeviceManager.h"

DeviceManager::DeviceManager(PhysicalDevice *device)
{
    _device = device;

    _deviceConfigs.push_back(new int[3] {0, device->getLedCount(), 0});
    _virtualDevices.push_back(new VirtualDevice(_device, 0, 0, _device->getLedCount()));
}

DeviceManager::DeviceManager(int ledCount)
{
    _device = new PhysicalDevice(ledCount);

    _deviceConfigs.push_back(new int[3] {0, ledCount, 0});
    _virtualDevices.push_back(new VirtualDevice(_device, 0, 0, ledCount)); // add a virtual device for the whole strip
}

DeviceManager::~DeviceManager()
{
    if (_device) {
        delete _device;
        _device = NULL;
    }

    for (int i = 0; i < _virtualDevices.size(); i--) {
        delete[] _deviceConfigs[i];
        delete _virtualDevices[i];
    }

    _deviceConfigs.clear();
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

int DeviceManager::addDevice(int offset, int ledCount, int zIndex, int mode)
{
    _deviceConfigs.push_back(new int[4] {offset, ledCount, zIndex, mode});

    std::vector<int*> configs = _deviceConfigs;

    std::vector<std::vector<int*>> hierarchy;

    while(configs.size()) {
        std::vector<int*> hierElem;

        for (int i = configs.size() - 1; i >= 0; i--) {
            if (configs[i][2] == hierarchy.size()) {
                hierElem.push_back(configs[i]);
                configs.erase(configs.begin() + i);
            }
        }

        hierarchy.push_back(hierElem);
    }

    
    for (int i = 0; i < hierarchy.size(); i++) {
        for (int j = 0; j < hierarchy[i].size(); j++) {
            
        }
    }


    int id = _virtualDevices.size(); // id is equal to the index
    _virtualDevices.push_back(new VirtualDevice(_device, id, offset, ledCount));
}

bool DeviceManager::editDevice(int id, int offset, int ledCount, int zIndex, int mode)
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