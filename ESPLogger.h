#ifndef LOGGER_H
#define LOGGER_H

#include <ESPAsyncTCP.h>
#include <Print.h>

class ESPLogger : public Print {
    private:
        AsyncServer *_telnetServer = NULL;
        AsyncClient *_telnet = NULL;

    public:
        ESPLogger();
        void begin();
        void update();

        size_t write(uint8_t);
        size_t write(const uint8_t *buffer, size_t size);
};

extern ESPLogger Logger;

#endif
