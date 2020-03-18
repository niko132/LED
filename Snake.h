#ifndef SNAKE_H
#define SNAKE_H

#include "Effect.h"
#include "Palette.h"

#include <FastLED.h>

class Snake : public Effect {
    public:
		Snake(Palette *palette) : Effect("Snake", palette) { };
	
        CRGB update(double timeValue, double posValue)
        {			
			CRGB color = _palette->getColorAtPosition(timeValue);
			
			double val = timeValue + posValue;
            val = val - (int) val;
			
			unsigned char brightness = 0;
			
			if (val <= 0.025) {
				brightness = val / 0.025d * 255;
			} else if (val <= 0.05) {
				brightness = 255;
			} else if (val <= 0.075) {
				brightness = ((0.075d - val) / 0.025d) * 255;
			}

            return color.nscale8(brightness);
        };
};

#endif // SNAKE_H