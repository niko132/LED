#include "ESPLogger.h"

ESPLogger::ESPLogger() : _telnetServer(23) {

};

void ESPLogger::begin() {
    _telnetServer.begin();
    _telnetServer.setNoDelay(true);

    update();
};

void ESPLogger::update() {
    if (_telnetServer.hasClient()) {
        if (!_telnet || !_telnet.connected()) {
            if (_telnet)
                _telnet.stop();

            _telnet = _telnetServer.available();
        } else {
            _telnetServer.available().stop();
        }
    }
};

size_t ESPLogger::write(uint8_t c) {
    return _telnet.write(c);
};

size_t ESPLogger::write(const uint8_t *buffer, size_t size) {
    return _telnet.write(buffer, size);
};

ESPLogger Logger;
