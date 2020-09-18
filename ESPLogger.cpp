#include "ESPLogger.h"

ESPLogger::ESPLogger() {
    _telnetServer = new AsyncServer(23);
};

void ESPLogger::begin() {
    _telnetServer->onClient([this](void *arg, AsyncClient *client) {
        _telnet = client;
    }, _telnetServer);
    _telnetServer->begin();

    update();
};

void ESPLogger::update() {

};

size_t ESPLogger::write(const uint8_t c) {
    if (_telnet)
        return _telnet->write((const char*) &c);

    return 0;
};

size_t ESPLogger::write(const uint8_t *buffer, size_t size) {
    if (_telnet)
        return _telnet->write((const char*) buffer, size);

    return 0;
};

ESPLogger Logger;
