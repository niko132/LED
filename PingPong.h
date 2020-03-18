#ifndef PINGPONG_H
#define PINGPONG_H

#include "Effect.h"
#include "Palette.h"

#include <FastLED.h>

class PingPong : public Effect {
    public:
		PingPong(Palette *palette) : Effect("PingPong", palette) { };
	
        CRGB update(double timeValue, double posValue)
        {
			CRGB color = _palette->getColorAtPosition(timeValue);
			
			if (timeValue > 0.5) {
				timeValue = 1.0 - timeValue;
			}
			
			double val = 0.075 + 2 * timeValue * 0.925 + posValue;
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

#endif // PINGPONG_H