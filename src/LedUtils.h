#ifndef __LEDUTILS_H__
#define __LEDUTILS_H__

#include "Adafruit_NeoPixel.h"

class LedUtils
{
public:
    /**
     * @brief Get a color from the actual light state
     * 
     * @param lightState The state to use to get the color
     * @return uint32_t A number containing the color information
     */
    static uint32_t Color(const LightState* lightState)
    {
        return Adafruit_NeoPixel::Color(lightState->red, lightState->green, lightState->blue, lightState->white);
    }

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

    /**
     * @brief Get the string for a effect
     * 
     * @param effect The effect to get the name for
     * @return const char* The name for the effect
     */
    static const char* EffectNameFromEnum(LightEffect effect)
    {
        switch (effect)
        {
        case LightEffect::solid:
            return "solid";
            break;
        case LightEffect::rainbow:
            return "rainbow";
            break;
        default:
            return "unknown";
            break;
        }
    }
};

#endif // __LEDUTILS_H__