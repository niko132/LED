#ifndef EFFECT_H
#define EFFECT_H

#include "Palette.h"

#include <FastLED.h>

class Effect {
    protected:
        Palette *_palette;

    public:
        Effect() {
            _palette = new Palette();
        };
        virtual CRGB update(double timeValue, double posValue) = 0;
};

#endif // EFFECT_H