#ifndef __LEDCONTROLLER_H__
#define __LEDCONTROLLER_H__

#include "Preferences.h"
#include "Adafruit_NeoPixel.h"
#include "ArduinoJson.h"

#define JSON_STATE_KEY "state"
#define JSON_BRIGHTNESS_KEY "brightness"
#define JSON_COLOR_MODE_KEY "color_mode"
#define JSON_COLOR_KEY "color"
#define JSON_RED_KEY "r"
#define JSON_GREEN_KEY "g"
#define JSON_BLUE_KEY "b"
#define JSON_WHITE_KEY "w"
#define JSON_EFFECT_KEY "effect"

#define ONBOARD_LED_PIN 18

#define EXTERNAL_LED_PIN 1
#define EXTERNAL_LED_LENGTH 150

#define FPS 1000 / 30

enum LightEffect 
{
    unknown,
    transition,
    solid,
    rainbow
};

struct LightStateUpdate
{
    bool lightOnPresent;
    bool lightOn;
    bool redPresent;
    byte red;
    bool greenPresent;
    byte green;
    bool bluePresent;
    byte blue;
    bool whitePresent;
    byte white;
    bool brightnessPresent;
    byte brightness;
    bool lightEffectPresent;
    LightEffect lightEffect;
};

struct LightState 
{
    bool lightOn;
    byte red;
    byte green;
    byte blue;
    byte white;
    byte brightness;
    LightEffect lightEffect;
    bool lightEffectChanged;
};


class LedController
{
private:
    Preferences* _preferences;
    LightState _lastState;
    LightState _state = { .lightOn = false, .red = 255, .green = 0, .blue = 0, .white = 0, .brightness = 100, .lightEffect = LightEffect::solid };

    Adafruit_NeoPixel _onboardLed;
    Adafruit_NeoPixel _externalLed;

    unsigned long nextExecution = 0;
    unsigned long pixelPrevious = 0;        // Previous Pixel Millis
    unsigned long patternPrevious = 0;      // Previous Pattern Millis
    int           patternCurrent = 0;       // Current Pattern Number
    int           patternInterval = 5000;   // Pattern Interval (ms)
    int           pixelInterval = 50;       // Pixel Interval (ms)
    int           pixelQueue = 0;           // Pattern Pixel Queue
    int           pixelCycle = 0;           // Pattern Pixel Cycle
    uint16_t      pixelCurrent = 0;         // Pattern Current Pixel Number
    uint16_t      pixelNumber = EXTERNAL_LED_LENGTH;  // Total Number of Pixels

public:
    LedController(Preferences* preferences);
    void setState(LightStateUpdate stateUpdate);
    const LightState* getState();
    void setBrightness(uint8_t newBrightness);
    void setColor(uint32_t newColor);
    void setLightEffect(LightEffect newEffect);
    void setOff();
    void setOn();
    void setup();
    void loop();
    void renderSolid();
    void renderRainbow();
};

#endif // __LEDCONTROLLER_H__