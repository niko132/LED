#include "PaletteManager.h"

PaletteManager::PaletteManager() {
	// TODO: create standard palettes
	
	// generate default rainbow palette
	std::vector<ColorKey> colors;
	
	for (int i = 0; i < 256; i++) {
		double pos = (double) i / 255.0;
		CRGB color = CHSV(i, 255, 255);

		colors.push_back({pos, color});
	}
	
	_palettes["rainbow"] = new Palette(colors);
};
		
Palette* PaletteManager::getPalette(String name, unsigned int resolution) {
	name.toLowerCase();
	
	std::map<String,Palette*>::iterator it = _palettes.find(name);
	
	if (it != _palettes.end()) {
		Palette *palette = it->second;
		
		if (palette->getResolution() < resolution) {
			palette->precalculate(resolution);
		}
		
		return palette;
	} else {
		// TODO: create Palette
		return NULL;
	}
}

PaletteManager Palettes;