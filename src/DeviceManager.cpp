#include "DeviceManager.h"

#include <stdio.h>
#include <iostream>

DeviceManager::DeviceManager(PhysicalDevice *device)
{
    _device = device;

    _deviceConfigs.push_back(new int[3] {0, device->getLedCount(), 0});
    _virtualDevices.push_back(new VirtualDevice(_device, 0, _device->getLedCount(), 0));
}

DeviceManager::DeviceManager(int ledCount)
{
    Serial.println("DeviceManager Constructor: " + String(ledCount));

    _device = new PhysicalDevice(ledCount);

/*
    _deviceConfigs.push_back(new int[3] {0, ledCount, 0});
    _virtualDevices.push_back(new VirtualDevice(_device, 0, ledCount, 0)); // add a virtual device for the whole strip
    */

   addDevice(0, ledCount, 0, 0);
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

int DeviceManager::addDevice(int startIndex, int endIndex, int zIndex, int mode)
{
    Serial.println("Adding Device1: " + String(startIndex) + " " + String(endIndex));

    VirtualDevice *newDevice = new VirtualDevice(_device, startIndex, endIndex, mode);
    addDevice(newDevice, zIndex);

    return newDevice->getId();
}

void DeviceManager::addDevice(VirtualDevice *newDevice, int zIndex) {
    int startIndex = newDevice->getStartIndex();
    int endIndex = newDevice->getEndIndex();

    Serial.println("Adding Device: " + String(startIndex) + " " + String(endIndex));

    if (zIndex >= _deviceHierarchy.size()) {
        _deviceHierarchy.push_back(new std::vector<VirtualDevice*>());
        zIndex = _deviceHierarchy.size() - 1;
    }

    Serial.println("Test2");

    for (int i = _deviceHierarchy[zIndex]->size() - 1; i >= 0; i--) {
        int start = _deviceHierarchy[zIndex]->at(i)->getStartIndex();
        int end = _deviceHierarchy[zIndex]->at(i)->getEndIndex();

        if (start < startIndex && end >= startIndex) {
            _deviceHierarchy[zIndex]->at(i)->setEnddIndex(startIndex);
        } else if (start <= endIndex && end > endIndex) {
            _deviceHierarchy[zIndex]->at(i)->setStartIndex(endIndex);
        } else if (start >= startIndex && end <= endIndex) {
            _deviceHierarchy[zIndex]->erase(_deviceHierarchy[zIndex]->begin() + i);
        }
    }

    Serial.println("Test3");

    for (int i = 0; i < _deviceHierarchy[zIndex]->size(); i++) {
        Serial.println("Hierarchy " + String(i) + ": " + String(_deviceHierarchy[zIndex]->at(i)->getStartIndex()) + " " + String(_deviceHierarchy[zIndex]->at(i)->getEndIndex()));
    }

    bool inserted = false;

    for (int i = 0; i < _deviceHierarchy[zIndex]->size(); i++) {
        Serial.println("For: " + String(i));

        if (startIndex < _deviceHierarchy[zIndex]->at(i)->getStartIndex()) {
            Serial.println("Inserting: " + String(i) + " " + String(newDevice->getId()));

            _deviceHierarchy[zIndex]->insert(_deviceHierarchy[zIndex]->begin() + i, newDevice);
            inserted = true;
            break;
        }
    }

    Serial.println("Test4");

    if (!inserted)
        _deviceHierarchy[zIndex]->push_back(newDevice);

    Serial.println("Size: " + String(_deviceHierarchy.size()) + " " + String(_deviceHierarchy[zIndex]->size()));

    buildDevices();
}

bool DeviceManager::editDevice(int id, int startIndex, int endIndex, int zIndex, int mode)
{
    for (int row = 0; row < _deviceHierarchy.size(); row++) {
        for (int col = 0; col < _deviceHierarchy[row]->size(); col++) {
            if (id == _deviceHierarchy[row]->at(col)->getId()) {
                editDevice(_deviceHierarchy[row]->at(col), startIndex, endIndex, zIndex, mode);
                return true;
            }
        }
    }

    return false;
}

void DeviceManager::editDevice(VirtualDevice *device, int startIndex, int endIndex, int zIndex, int mode)
{
    removeDevice(device);

    device->setStartIndex(startIndex);
    device->setEnddIndex(endIndex);
    device->setMode(mode);

    addDevice(device, zIndex);
}

bool DeviceManager::removeDevice(int id)
{
    for (int row = 0; row < _deviceHierarchy.size(); row++) {
        for (int col = 0; col < _deviceHierarchy[row]->size(); col++) {
            if (id == _deviceHierarchy[row]->at(col)->getId()) {
                return removeDevice(_deviceHierarchy[row]->at(col));
            }
        }
    }

    return false;
}

bool DeviceManager::removeDevice(VirtualDevice *device)
{
    bool found = false;

    for (int row = _deviceHierarchy.size() - 1; row >= 0; row--) {
        for (int col = 0; col < _deviceHierarchy[row]->size(); col++) {
            if (_deviceHierarchy[row]->at(col) == device) {
                _deviceHierarchy[row]->erase(_deviceHierarchy[row]->begin() + col);
                found = true;
                break;
            }
        }

        if (_deviceHierarchy[row]->empty()) {
            _deviceHierarchy.erase(_deviceHierarchy.begin() + row);
        }
    }

    buildDevices();

    return found;
}

void DeviceManager::buildDevices() {
    for (int row = _deviceHierarchy.size() - 1; row >= 0; row--) {
        for (int col = 0; col < _deviceHierarchy[row]->size(); col++) {
            _deviceHierarchy[row]->at(col)->resetAreas();

            for (int innerRow = _deviceHierarchy.size() - 1; innerRow > row; innerRow--) {
                for (int innerCol = 0; innerCol < _deviceHierarchy[innerRow]->size(); innerCol++) {
                    int start = _deviceHierarchy[innerRow]->at(innerCol)->getStartIndex();
                    int end = _deviceHierarchy[innerRow]->at(innerCol)->getEndIndex();

                    _deviceHierarchy[row]->at(col)->removeArea(start, end);
                }
            }
        }
    }
}

void DeviceManager::update()
{
    for (int row = 0; row < _deviceHierarchy.size(); row++) {
        for (int col = 0; col < _deviceHierarchy[row]->size(); col++) {
            _deviceHierarchy[row]->at(col)->update();
        }
    }
}

void DeviceManager::debug()
{
    for (int row = 0; row < _deviceHierarchy.size(); row++) {
        for (int col = 0; col < _deviceHierarchy[row]->size(); col++) {
            std::cout << row << ":    ";

/*
            for (int i = 0; i < _device->getLedCount(); i++) {
                if (i >= _deviceHierarchy[row]->at(col)->getStartIndex() && i < _deviceHierarchy[row]->at(col)->getEndIndex())
                    std::cout << i << " ";
                else
                    std::cout << "*";
            }
            */

           _deviceHierarchy[row]->at(col)->debug();

            std::cout << std::endl;
        }
    }
}