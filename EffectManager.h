#ifndef EFFECTMANAGER_H
#define EFFECTMANAGER_H

#include "Effect.h"
#include "CustomEffect.h"
#include "Palette.h"

#include <FS.h>

class EffectManager {
	private:
		const unsigned int NUM_LOCAL_EFFECTS = 5;
	
		int getLocalEffectIndex(String name);
		CustomEffect* getEffect(File f, Palette *palette);
		String getShortFileName(String filename);
	
	public:	
		unsigned int getEffectCount();
		Effect* getEffect(String name, Palette *palette);
		Effect* getEffectAt(unsigned int index, Palette *palette);
		
		String getEffectNameAt(unsigned int index);
		
		int createEffect(CustomEffect *effect);
		void deleteEffect(String name);
};

extern EffectManager LEDEffectManager;

#endif // EFFECTMANAGER_H