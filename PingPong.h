#ifndef PINGPONG_H
#define PINGPONG_H

#include "Effect.h"
#include "Palette.h"

#include <FastLED.h>

class PingPong : public Effect {
	private:
		double _lightLength = 0.2;
	
    public:
		PingPong(Palette *palette) : Effect("PingPong", palette) { };
	
        CRGB update(double timeValue, double posValue)
        {
			CRGB color = _palette->getColorAtPosition(timeValue);
			
			if (timeValue > 0.5) {
				timeValue = 1.0 - timeValue;
			}			
			
			double val = _lightLength + 2 * timeValue * (1.0 - _lightLength) + posValue;
			val = val - (int) val;
			
			unsigned char brightness = 0;
			
			if (val <= _lightLength) {
				if (val > _lightLength / 5.0 * 3.0)
					val = _lightLength - val;
				
				double tmp = 1.0;
				
				if (val <= _lightLength / 5.0 * 2.0) {
					tmp = val / (_lightLength / 5.0 * 2.0);
					tmp = tmp * tmp * tmp;
				}
				
				brightness = tmp * 255;
			}
			
			/*			
			if (val <= _lightLength) {
				if (val > _lightLength / 2.0) {
					val = _lightLength - val;
				}
				
				double tmp = val / (_lightLength / 2.0);
				brightness = tmp * tmp * tmp * 255;
			}
			*/

            return color.nscale8_video(brightness);
        };
};

#endif // PINGPONG_H