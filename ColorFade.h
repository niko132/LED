#ifndef COLORFADE_H
#define COLORFADE_H

#include "Effect.h"
#include "Palette.h"

#include <FastLED.h>

class ColorFade : public Effect {
	public:
		ColorFade(Palette *palette) : Effect("ColorFade", palette) { };
	
		CRGB update(double timeValue, double posValue)
		{
			timeValue = timeValue - (int) timeValue;
			
			return _palette->getColorAtPosition(timeValue);
		};
};

#endif // COLORFADE_H