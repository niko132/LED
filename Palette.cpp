#include "Palette.h"

Palette::Palette()
{
	
}

Palette::Palette(std::vector<ColorKey> colors)
{
	_colors = new std::vector<ColorKey>(colors);
}

Palette::~Palette()
{
	if (_precalculated) {
		delete[] _precalculated;
		_precalculated = NULL;
	}
	
	if (_colors) {
		delete _colors;
		_colors = NULL;
	}
}

unsigned int Palette::getResolution()
{
	return _resolution;
}

void Palette::precalculate(int maxResolution)
{
	if (!_colors)
		return;
	
	if (_precalculated) {
		delete[] _precalculated;
		_precalculated = NULL;
	}
	
	_precalculated = new unsigned char[maxResolution * 3];

    if (_colors->size() == 0) {
        memset(_precalculated, 0, maxResolution * 3);
        return;
    }
	
	for (int i = 0; i < maxResolution; i++) {
		float value = (float) i / maxResolution;
	
		int index1 = _colors->size() - 1;
				
		for (int i = 0; i < _colors->size(); i++) {
			if (_colors->at(i).pos > value)
				break;
					
			index1 = i;
		}
				
		int index2 = (index1 + 1) % _colors->size();
				
		CRGB color1 = _colors->at(index1).color;
		CRGB color2 = _colors->at(index2).color;
				
		float frac1 = value - _colors->at(index1).pos;
				
		if (frac1 < 0)
			frac1 += 1.0f;
				
		float frac2 = _colors->at(index2).pos - _colors->at(index1).pos;
				
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
	
	_resolution = maxResolution;
	
	
	delete _colors;
	_colors = NULL;
}

CRGB Palette::getColorAtPosition(double pos) {
	// TODO: increase resolution -> keep heap in mind!!!
	unsigned int resolution = 180;
    double brightness = 1.0;
	
	unsigned int index = pos * resolution;
	
	if (!_precalculated) {
		precalculate(resolution);
    }
	
	unsigned char *c = &_precalculated[index * 3];
	
	return CRGB(c[0], c[1], c[2]).nscale8(brightness * 255);
};