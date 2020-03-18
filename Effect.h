#ifndef EFFECT_H
#define EFFECT_H

#include "Palette.h"

#include <FastLED.h>

class Effect {
    protected:
		String _name;
        Palette *_palette;

    public:
        Effect(String name, Palette *palette) {
			_name = name;
			_palette = palette;
        };
		
		String getName() {
			return _name;
		};
		
        virtual CRGB update(double timeValue, double posValue) = 0;
};

#endif // EFFECT_H