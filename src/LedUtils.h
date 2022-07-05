#ifndef __LEDUTILS_H__
#define __LEDUTILS_H__

#include "Adafruit_NeoPixel.h"

class LedUtils
{
public:
    /**
     * @brief Get a single value containing every Color, by a color from the color wheel
     * 
     * @param wheelPos The position of the color wheel to use 0-255
     * @return uint32_t A color value containing R,G & B
     */
    static uint32_t ColorFromWheel(byte wheelPos)
    {
        wheelPos = 255 - wheelPos;
        
        if(wheelPos < 85) 
        {
            return Adafruit_NeoPixel::Color(255 - wheelPos * 3, 0, wheelPos * 3);
        }
        
        if(wheelPos < 170) 
        {
            wheelPos -= 85;
            return Adafruit_NeoPixel::Color(0, wheelPos * 3, 255 - wheelPos * 3);
        }
        
        wheelPos -= 170;
        
        return Adafruit_NeoPixel::Color(wheelPos * 3, 255 - wheelPos * 3, 0);
    }
};

#endif // __LEDUTILS_H__