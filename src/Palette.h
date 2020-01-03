#ifndef PALETTE_H
#define PALETTE_H

#include <FastLED.h>
#include <vector>

typedef struct {
    double pos;
    CRGB color;
} ColorKey;

class Palette {
    private:
        std::vector<ColorKey> _colors;
        unsigned char *_precalculated;

    public:
        void precalculate(int maxResolution);
        CRGB getColorAtPosition(double pos);
};

#endif // PALETTE_H