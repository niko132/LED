#ifndef COLORCYCLE_H
#define COLORCYCLE_H

#include "Effect.h"
#include "Palette.h"

#include <FastLED.h>

class ColorCycle : public Effect {
    public:
		ColorCycle(Palette *palette) : Effect("ColorCycle", palette) { };
	
        CRGB update(double timeValue, double posValue)
        {
            double val = timeValue + posValue;
            val = val - (int) val;

            return _palette->getColorAtPosition(val);
        };
};

#endif // COLORCYCLE_H