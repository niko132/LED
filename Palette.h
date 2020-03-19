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
        std::vector<ColorKey> *_colors;
		unsigned int _resolution = 0;
        unsigned char *_precalculated = NULL;

    public:
		Palette();
		Palette(std::vector<ColorKey> colors);
		~Palette();
		
		unsigned int getResolution();
        void precalculate(int maxResolution);
        CRGB getColorAtPosition(double pos);
};

#endif // PALETTE_H