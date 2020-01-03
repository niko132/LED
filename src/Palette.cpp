#include "Palette.h"

void Palette::precalculate(int maxResolution)
{
	_precalculated = new unsigned char[maxResolution * 3];

    if (_colors.size() == 0) {
        memset(_precalculated, 0, maxResolution * 3);
        return;
    }
	
	for (int i = 0; i < maxResolution; i++) {
		float value = (float) i / maxResolution;
	
		int index1 = _colors.size() - 1;
				
		for (int i = 0; i < _colors.size(); i++) {
			if (_colors[i].pos > value)
				break;
					
			index1 = i;
		}
				
		int index2 = (index1 + 1) % _colors.size();
				
		CRGB color1 = _colors[index1].color;
		CRGB color2 = _colors[index2].color;
				
		float frac1 = value - _colors[index1].pos;
				
		if (frac1 < 0)
			frac1 += 1.0f;
				
		float frac2 = _colors[index2].pos - _colors[index1].pos;
				
		if (frac2 <= 0)
			frac2 += 1.0f;
				
		float frac = frac1 / frac2;
				
		CRGB color = nblend(color1, color2, frac * 255);
		
		int b = color.r + color.g + color.b;
		
		if (b > 255)
			b = 255;
		
		color.nscale8_video(b);
		color.nscale8_video(b);
		color.nscale8_video(b);
		
		_precalculated[3 * i] = color.r;
		_precalculated[3 * i + 1] = color.g;
		_precalculated[3 * i + 2] = color.b;
	}
}

CRGB Palette::getColorAtPosition(double pos) {	
	unsigned int resolution = 256;
    double brightness = 1.0;
	
	unsigned int index = pos * resolution;
	
	if (!_precalculated) {
		precalculate(resolution);
    }
	
	unsigned char *c = &_precalculated[index * 3];
	
	return CRGB(c[0], c[1], c[2]).nscale8(brightness * 255);
};