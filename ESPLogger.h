#ifndef LOGGER_H
#define LOGGER_H

#include <ESP8266WiFi.h>

class ESPLogger : public Print {
    private:
        WiFiServer _telnetServer;
        WiFiClient _telnet;

    public:
        ESPLogger();
        void begin();
        void update();

        size_t write(uint8_t);
        size_t write(const uint8_t *buffer, size_t size);
};

extern ESPLogger Logger;

#endif
