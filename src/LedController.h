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

#define ONBOARD_LED_PIN 18

#define EXTERNAL_LED_PIN 1
#define EXTERNAL_LED_LENGTH 150

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
};

struct LightState 
{
    bool lightOn;
    byte red;
    byte green;
    byte blue;
    byte white;
    byte brightness;
};


class LedController
{
private:
    Preferences* _preferences;
    LightState _state = { .lightOn = false, .red = 255, .green = 0, .blue = 0, .white = 0, .brightness = 100 };

    Adafruit_NeoPixel _onboardLed;
    Adafruit_NeoPixel _externalLed;


public:
    LedController(Preferences* preferences);
    void setState(LightStateUpdate stateUpdate);
    const LightState* getState();
    void setup();
    void loop();
};

#endif // __LEDCONTROLLER_H__