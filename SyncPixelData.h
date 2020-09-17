#ifndef SYNCPIXELDATA_H
#define SYNCPIXELDATA_H

#include "SyncAlgorithm.h"

class syncPixelData : public SyncAlgorithm {
    protected:
        unsigned char *_pixelData = NULL;

    public:
        SyncAlgorithm(unsigned long timeOffset, Effect *effect, unsigned int length) : SyncAlgorithm(timeOffset, effect, length) {
            _pixelData = new unsigned char[length * 3];
        };

        ~SyncAlgorithm() {
            if (_pixelData) {
                delete[] _pixelData;
                _pixelData = NULL;
            }

            _length = 0;
        };

        void setLength(unsigned int length) {
            SyncAlgorithm::setLength(length);

            if (_pixelData) {
                delete[] _pixelData;
                _pixelData = NULL;
            }

            _pixelData = new unsigned char[length * 3];
        };

        void syncPixelData(unsigned char *pixelData unsigned int length) {
            if (_length * 3 < length) {
                length = _length * 3; // _length is the buffer size -> max char count
            }

            memcpy(_pixelData, pixelData, length);
        };

        CRGB updatePixel(double timeValue, double posValue, unsigned int relIndex, Effect *effect) {
            if (_pixelData && relIndex < _length) {
                unsigned char r = _pixelData[relIndex * 3];
                unsigned char g = _pixelData[relIndex * 3 + 1];
                unsigned char b = _pixelData[relIndex * 3 + 2];

                return CRGB(r, g, b);
            }

            return CRGB(0, 0, 0);
        };
};

#endif
