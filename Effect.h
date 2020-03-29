#ifndef EFFECT_H
#define EFFECT_H

#include "Palette.h"

#include <FastLED.h>

class Effect {
    protected:
		String _name;
        Palette *_palette = NULL;

    public:	
        Effect(String name, Palette *palette) {
			_name = name;
			_palette = palette;
        };
		
		String getName() {
			return _name;
		};
		
		virtual double getDuration() {
			return 5.0; // default is 5 seconds
		};
		
		void setName(String name)
		{
			_name = name;
		};
		
		void setPalette(Palette *palette) {
			_palette = palette;
		};
		
        virtual CRGB update(double timeValue, double posValue) = 0;
};

#endif // EFFECT_H