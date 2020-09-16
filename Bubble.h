#ifndef BUBBLE_H
#define BUBBLE_H

#include "Effect.h"
#include "Palette.h"

#include <FastLED.h>

class Bubble : public Effect {
	private:
		double _lightLength = 0.2;
		
		const unsigned char FADE[256] = { 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 9, 9, 9, 9, 10, 10, 10, 10, 11, 11, 11, 11, 11, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 15, 15, 15, 15, 16, 16, 16, 17, 17, 17, 18, 18, 19, 19, 19, 20, 20, 20, 21, 21, 22, 22, 23, 23, 23, 24, 24, 25, 25, 26, 26, 27, 27, 28, 28, 29, 29, 30, 30, 31, 31, 32, 32, 33, 34, 34, 35, 35, 36, 37, 37, 38, 39, 39, 40, 41, 41, 42, 43, 44, 44, 45, 46, 47, 48, 48, 49, 50, 51, 52, 53, 54, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 67, 68, 69, 70, 71, 72, 73, 75, 76, 77, 78, 80, 81, 82, 84, 85, 86, 88, 89, 91, 92, 94, 95, 97, 98, 100, 102, 103, 105, 107, 108, 110, 112, 114, 116, 117, 119, 121, 123, 125, 127, 129, 131, 134, 136, 138, 140, 142, 145, 147, 149, 152, 154, 157, 159, 162, 164, 167, 169, 172, 175, 178, 180, 183, 186, 189, 192, 195, 198, 202, 205, 208, 211, 215, 218, 221, 225, 229, 232, 236, 240, 243, 247, 251, 255 };
		const unsigned char SIGMOID[256] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 8, 8, 9, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 15, 15, 16, 17, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 32, 33, 34, 36, 37, 39, 40, 42, 44, 45, 47, 49, 51, 53, 55, 57, 59, 61, 63, 66, 68, 70, 73, 75, 78, 80, 83, 86, 88, 91, 94, 97, 99, 102, 105, 108, 111, 114, 117, 120, 123, 126, 129, 132, 135, 138, 141, 144, 147, 150, 153, 156, 158, 161, 164, 167, 169, 172, 175, 177, 180, 182, 185, 187, 189, 192, 194, 196, 198, 200, 202, 204, 206, 208, 210, 211, 213, 215, 216, 218, 219, 221, 222, 223, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 238, 239, 240, 240, 241, 242, 242, 243, 243, 244, 244, 245, 245, 246, 246, 246, 247, 247, 248, 248, 248, 249, 249, 249, 249, 250, 250, 250, 250, 251, 251, 251, 251, 251, 251, 252, 252, 252, 252, 252, 252, 252, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254 };
	
    public:
		Bubble(Palette *palette) : Effect("Bubble", palette) { };
		
		unsigned char sigmoid(double in)
		{
			if (in < -6.0)
				return 0;
			
			if (in > 6.0)
				return 255;
			
			int index = (in + 6.0) / 12.0 * 255.0;
			return SIGMOID[index];
		}
		
		unsigned char getValue(double posValue, double timeValue, double offset)
		{
			double fac = 24.0;
			double val = fac * (posValue - offset) - fac * timeValue;
			double sig = sigmoid(val);
			return 4.0 * sig * (255 - sig) / 255.0;
		}
		
		CRGB getColorBlob(CRGB color, double timeValue, double posValue, double blobPos)
		{
			if (timeValue >= 0.5)
				timeValue = 1.0 - timeValue;
			
			timeValue *= 2.0;
			
			unsigned int val1 = getValue(posValue, blobPos, 0.0);
			unsigned int val2 = getValue(posValue, blobPos, -1.0);
			unsigned int val3 = getValue(posValue, blobPos, 1.0);
			
			double val4 = val1 + val2 + val3;
			
			if (val4 > 255.0)
				val4 = 255.0;
			
			val4 *= timeValue;
			
			double val = FADE[(unsigned char) val4];

            return color.nscale8_video(val);
		}
		
		unsigned char func(unsigned char in, unsigned char pos, unsigned char width)
		{
			// unsigned char begin = (255 - width) / 2.0;
			unsigned char begin = 0;
			unsigned char end = width;
			
			in += width / 2.0; // translate hill max to the origin
			in -= pos;
			
			if (in >= end)
				return 0;
			
			in -= begin;
			in *= 255.0 / width;
			
			if (in >= 128)
				in = 255 - in;
			
			in *= 2;
			
			return ease8InOutQuad(in);
		}
		
		CRGB createBlob(unsigned char in, unsigned char pos, unsigned char width, CRGB color)
		{
			unsigned char val = func(in, pos, (unsigned char) (width * 1.5));
			return color.nscale8_video(val);
		}
	
        CRGB update(double timeValue, double posValue)
        {
			/*
			CRGB red = _palette->getColorAtPosition(0.0);
			CRGB blue = _palette->getColorAtPosition(0.666666666);
			*/
			
			unsigned char brightness = func(timeValue * 255, 0, 255);
			
			CRGB combined = CRGB(0, 0, 0);
			
			const unsigned int numBlobs = 2;
			
			CRGB colors[numBlobs];
			colors[0] = CRGB(255, 0, 0);
			colors[1] = CRGB(0, 0, 255);
			
			for (int i = 0; i < numBlobs; i++) {
				CRGB color = colors[i];
				color = createBlob(posValue * 255, (unsigned char) (255.0 / numBlobs / 2.0 + i * 255.0 / numBlobs + 0.5), (unsigned char) ((double) brightness / numBlobs + 0.5), color);
				combined += color;
			}
			
			/*
			CRGB red = CRGB(255, 0, 0);
			CRGB blue = CRGB(0, 0, 255);
			
			CRGB blob1 = createBlob(posValue * 255, 64, red);
			CRGB blob2 = createBlob(posValue * 255, 192, blue);
			
			CRGB combined = blob1 + blob2;
			*/
			
			return combined.nscale8_video(brightness);
		};
};

#endif // BUBBLE_H