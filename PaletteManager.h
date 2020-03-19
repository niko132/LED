#ifndef PALETTEMANAGER_H
#define PALETTEMANAGER_H

#include "Palette.h"

#include <map>

class PaletteManager {
	private:
		std::map<String, Palette*> _palettes;
		
	public:
		PaletteManager();
		Palette* getPalette(String name, unsigned int resolution);
};

extern PaletteManager Palettes;

#endif // PALETTEMANAGER_H