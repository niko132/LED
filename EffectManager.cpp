#include "EffectManager.h"

#include "ColorCycle.h"
#include "ColorFade.h"
#include "StaticColor.h"
#include "Snake.h"
#include "PingPong.h"
#include "Bubble.h"
#include "CustomEffect.h"

unsigned int EffectManager::getEffectCount()
{
	Dir fxDir = SPIFFS.openDir("fx");

	unsigned int count = NUM_LOCAL_EFFECTS; // this is the number of locally available effects

	while(fxDir.next()) {
		count++;
	}

	return count;
}

int EffectManager::getLocalEffectIndex(String name)
{
	if (name == "ColorCycle")
		return 0;
	else if (name == "ColorFade")
		return 1;
	else if (name == "StaticColor")
		return 2;
	else if (name == "Snake")
		return 3;
	else if (name == "PingPong")
		return 4;
	else if (name == "Bubble")
		return 5;

	return -1;
}

CustomEffect* EffectManager::getEffect(File f, Palette *palette)
{
	if (!f)
		return NULL;

	String name = f.name();

	CustomEffect *effect = new CustomEffect(palette, &f, name);

	f.close();

	return effect;
}

String EffectManager::getShortFileName(String filename)
{
	int extIndex = filename.lastIndexOf('.');
	if (extIndex == -1)
		return filename;

	filename = filename.substring(0, extIndex);

	int slashIndex = filename.lastIndexOf('/');
	if (slashIndex != -1)
		filename = filename.substring(slashIndex + 1);

	return filename;
}

Effect* EffectManager::getEffect(String name, Palette *palette)
{
	if (name.length() <= 0) { // empty name -> default effect
		return getDefaultEffect(palette);
	}

	int index = getLocalEffectIndex(name);
	if (index >= 0)
		return getEffectAt(index, palette);

	String filename = "fx/" + name + ".fx";
	File fxFile = SPIFFS.open(filename, "r");

	return getEffect(fxFile, palette);
}

Effect* EffectManager::getEffectAt(unsigned int index, Palette *palette)
{
	switch (index) {
		case 0:
			return new ColorCycle(palette);
		case 1:
			return new ColorFade(palette);
		case 2:
			return new StaticColor(palette);
		case 3:
			return new Snake(palette);
		case 4:
			return new PingPong(palette);
		case 5:
			return new Bubble(palette);
	}

	Dir fxDir = SPIFFS.openDir("fx");

	File fxFile;
	unsigned int count = NUM_LOCAL_EFFECTS;

	while(fxDir.next()) {
		if (count == index) {
			fxFile = fxDir.openFile("r");
			break;
		}

		count++;
	}

	return getEffect(fxFile, palette);
}

Effect* EffectManager::getDefaultEffect(Palette *palette) {
	return getEffectAt(0, palette);
}

String EffectManager::getEffectNameAt(unsigned int index)
{
	switch (index) {
		case 0:
			return "ColorCycle";
		case 1:
			return "ColorFade";
		case 2:
			return "StaticColor";
		case 3:
			return "Snake";
		case 4:
			return "PingPong";
		case 5:
			return "Bubble";
	}

	Dir fxDir = SPIFFS.openDir("fx");

	File fxFile;
	unsigned int count = NUM_LOCAL_EFFECTS;

	while(fxDir.next()) {
		if (count == index) {
			String name = fxDir.fileName();
			return getShortFileName(name);
		}

		count++;
	}

	return "";
}

int EffectManager::createEffect(CustomEffect *effect)
{
	String name = effect->getName();

	String filename = "fx/" + name + ".fx";
	File fxFile = SPIFFS.open(filename, "w");

	effect->serialize(&fxFile);

	fxFile.close();


	Dir fxDir = SPIFFS.openDir("fx");

	unsigned int index = NUM_LOCAL_EFFECTS;

	while(fxDir.next()) {
		String filename = fxDir.fileName();
		if (getShortFileName(filename) == name)
			return index;

		index++;
	}

	return -1;
}

void EffectManager::deleteEffect(String name)
{
	String filename = "fx/" + name + ".fx";
	SPIFFS.remove(filename);
}

EffectManager LEDEffectManager;
