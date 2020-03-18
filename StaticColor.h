#ifndef STATICCOLOR_H
#define STATICCOLOR_H

#include "Effect.h"
#include "Palette.h"

#include <FastLED.h>

class StaticColor : public Effect {
	public:
		StaticColor(Palette *palette) : Effect("StaticColor", palette) { };
	
		CRGB update(double timeValue, double posValue)
		{
			// just return the first color of the palette for every index
			return _palette->getColorAtPosition(0.5);
		};
};

#endif // STATICCOLOR_H