#include "LedController.h"
#include "LedUtils.h"

LedController::LedController(Preferences* preferences) : 
    _onboardLed(1, ONBOARD_LED_PIN, NEO_GRB + NEO_KHZ800),
    _externalLed(EXTERNAL_LED_LENGTH, EXTERNAL_LED_PIN, NEO_GRBW + NEO_KHZ800)
{
    _preferences = preferences;
}

void LedController::setState(LightStateUpdate stateUpdate)
{
    Serial.println(F("\nLed controller state will be updated"));

    if (stateUpdate.brightnessPresent)
    {
#if DEBUG_LIGHT
        Serial.println(F("There is brightness information"));
#endif
        _state.brightness = stateUpdate.brightness;
        setBrightness(_state.brightness);
    }

    if (stateUpdate.redPresent || stateUpdate.greenPresent || stateUpdate.bluePresent || stateUpdate.whitePresent)
    {
#if DEBUG_LIGHT
        Serial.println(F("There is some color information"));
#endif
        _state.red = stateUpdate.red;
        _state.green = stateUpdate.green;
        _state.blue = stateUpdate.blue;
        _state.white = stateUpdate.white;
        
        uint32_t color = Adafruit_NeoPixel::Color(_state.red, _state.green, _state.blue, _state.white);
        setColor(color);
    }
    
    if (stateUpdate.lightEffectPresent)
    {
#if DEBUG_LIGHT
        Serial.println(F("There is light effect information"));
#endif
        setLightEffect(stateUpdate.lightEffect);
    }

    if (stateUpdate.lightOnPresent)
    {
#if DEBUG_LIGHT
        Serial.println(F("There is state information"));
#endif
        _state.lightOn = stateUpdate.lightOn;

        if (_state.lightOn && !_lastState.lightOn){
            setOn();
        }
        else if (!_state.lightOn && _lastState.lightOn){
            setOff();
        }
    }   
}

const LightState* LedController::getState()
{
    return &_state;
}

void LedController::setBrightness(uint8_t newBrightness)
{
#if DEBUG_LIGHT
    Serial.printf("Brightness: %d\n", _state.brightness);
#endif
    _onboardLed.setBrightness(_state.brightness);
    _externalLed.setBrightness(_state.brightness);
}

void LedController::setColor(uint32_t newColor)
{
#if DEBUG_LIGHT
    Serial.printf("R: %d; G: %d, B: %d, W: %d\n", _state.red, _state.green, _state.blue, _state.white);
#endif
    
    _onboardLed.setPixelColor(0, LedUtils::Color(&_state));

    _onboardLed.show();

    for (size_t i = 0; i < EXTERNAL_LED_LENGTH; i++)
    {
        _externalLed.setPixelColor(i, LedUtils::Color(&_state));
    }

    _externalLed.show();
}

void LedController::setLightEffect(LightEffect newEffect)
{
    if (_state.lightEffect == newEffect)
        return; // The effect did not change

#if DEBUG_LIGHT
    Serial.printf("light effect changed to: '%s'\n", LedUtils::EffectNameFromEnum(newEffect));
#endif

    _state.lightEffect = newEffect;
    _state.lightEffectChanged = true;
}

void LedController::setOff()
{
#if DEBUG_LIGHT
    Serial.println(F("light turned off"));
#endif
    
    // light switched off
    _onboardLed.setPixelColor(0, 0, 0, 0, 0);
    _onboardLed.show();

    for (size_t i = 0; i < EXTERNAL_LED_LENGTH; i++)
    {
        _externalLed.setPixelColor(i, 0, 0, 0, 0);
    }

    _externalLed.show();
}

void LedController::setOn()
{
#if DEBUG_LIGHT
    Serial.println(F("light turned on"));
#endif

    _onboardLed.setPixelColor(0, _state.red, _state.green, _state.blue, _state.white);
    _onboardLed.show();

    for (size_t i = 0; i < EXTERNAL_LED_LENGTH; i++)
    {
        _externalLed.setPixelColor(i, LedUtils::Color(&_state));
    }

    _externalLed.show();
}

void LedController::setup()
{
    _onboardLed.setBrightness(_state.brightness);
    _externalLed.setBrightness(_state.brightness);
    _onboardLed.begin();
    _externalLed.begin();

    setLightEffect(LightEffect::solid);
}

void LedController::loop()
{
    auto now = millis();

    if (now < nextRenderExecution)
        return;

    nextRenderExecution = now + 4;

    if(_state.lightOn)
    {
        switch (_state.lightEffect)
        {
        case LightEffect::solid:
            renderSolid();
            break;
        case LightEffect::rainbow:
            renderRainbow();
            break;
        default:
            break;
        }
    }

    // save the state for the next frame (edge detection)
    _lastState = _state;
}

void LedController::renderSolid()
{
    if (_state.lightEffectChanged)
    {
        _onboardLed.setPixelColor(0, _state.red, _state.green, _state.blue, _state.white);
        _onboardLed.show();

        for (size_t i = 0; i < EXTERNAL_LED_LENGTH; i++)
        {
            _externalLed.setPixelColor(i, _state.red, _state.green, _state.blue, _state.white);
        }
        _externalLed.show(); 

        _state.lightEffectChanged = false;
    }
}

void LedController::renderRainbow()
{
    _state.lightEffectChanged = false;

    for(uint16_t i=0; i < pixelNumber; i++) 
    {
        //_externalLed.setPixelColor(i, Adafruit_NeoPixel::ColorHSV((i + pixelCycle) & 255));
        _externalLed.setPixelColor(i, LedUtils::ColorFromWheel((i + pixelCycle) & 255)); //  Update delay time  
    }

    _externalLed.show();                             //  Update strip to match
    pixelCycle++;           //  Advance current cycle
    if(pixelCycle >= 256)
        pixelCycle = 0;                         //  Loop the cycle back to the begining   
}

void LedController::renderDot(unsigned long now, bool reverse)
{
    static bool up = true;
    static size_t index = 0;

    // turn any led of at the beginning
    if (_state.lightEffectChanged)
    {
        _state.lightEffectChanged = false;

        for(size_t i=0; i < EXTERNAL_LED_LENGTH; i++) 
        {
            _externalLed.setPixelColor(i, 0);
        }

        up = true;
        index = 0;
    }

    if (now >= nextExecution)
    {
        nextExecution = now + 1;

        _externalLed.setPixelColor(index, LedUtils::Color(&_state));
        _externalLed.show();
        // turn it off
        _externalLed.setPixelColor(index, 0);

        if (up)
        {
            if (index < (EXTERNAL_LED_LENGTH - 1))
            {
                index ++;
            }
            else
            {
                if (reverse)
                {
                    up = false;
                }
                else
                {
                    index = 0;
                }
            }
        }
        else
        {
            if (index > 0)
            {
                index --;
            }
            else{
                up = true;
            }
        }
    }
}

void LedController::renderDotTrace(unsigned long now, bool reverse)
{
    renderDot(now, reverse);
}
