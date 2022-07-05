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
    Serial.println("\nLed controller state will be updated");

    if (stateUpdate.lightOnPresent)
    {
        _state.lightOn = stateUpdate.lightOn;
        Serial.printf("Light switched: %d\n", _state.lightOn);
    }   

    if (stateUpdate.brightnessPresent)
    {
        _state.brightness = stateUpdate.brightness;
        _onboardLed.setBrightness(_state.brightness);
        _externalLed.setBrightness(_state.brightness);
        Serial.printf("Brightness: %d\n", _state.brightness);
    }

    if (stateUpdate.redPresent)
    {
        _state.red = stateUpdate.red;
        Serial.printf("Red: %d\n", _state.red);
    }
    
    if (stateUpdate.greenPresent)
    {
        _state.green = stateUpdate.green;
        Serial.printf("Green: %d\n", _state.green);
    }
    
    if (stateUpdate.bluePresent)
    {
        _state.blue = stateUpdate.blue;
        Serial.printf("Blue: %d\n", _state.blue);
    }

    if (stateUpdate.whitePresent)
    {
        _state.white = stateUpdate.white;
        Serial.printf("White: %d\n", _state.white);
    }

    if (stateUpdate.lightEffectPresent)
    {
        setLightEffect(stateUpdate.lightEffect);
    }
}

const LightState* LedController::getState()
{
    return &_state;
}

void LedController::setLightEffect(LightEffect newEffect)
{
    if (_state.lightEffect == newEffect)
        return; // The effect did not change

    _state.lightEffect = newEffect;
    _state.lightEffectChanged = true;
}

void LedController::setup()
{
    _onboardLed.setBrightness(_state.brightness);
    _externalLed.setBrightness(_state.brightness);
    _onboardLed.begin();
    _externalLed.begin();
}

void LedController::loop()
{
    // rising edge detection
    if (_state.lightOn && !_lastState.lightOn)
    {
        // light switched on

        _onboardLed.setPixelColor(0, _state.red, _state.green, _state.blue, _state.white);
        _onboardLed.show();
    }
    else if (!_state.lightOn && _lastState.lightOn)
    {
        // light switched off
        _onboardLed.setPixelColor(0, 0, 0, 0, 0);
        _onboardLed.show();

        for (size_t i = 0; i < EXTERNAL_LED_LENGTH; i++)
        {
            _externalLed.setPixelColor(i, 0, 0, 0, 0);
        }

        _externalLed.show();
    }
    else if (_state.lightOn)
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
    // if(pixelInterval != wait)
    //     pixelInterval = wait;

    for(uint16_t i=0; i < pixelNumber; i++) 
    {
        _externalLed.setPixelColor(i, LedUtils::ColorFromWheel((i + pixelCycle) & 255)); //  Update delay time  
    }

    _externalLed.show();                             //  Update strip to match
    pixelCycle++;           //  Advance current cycle
    if(pixelCycle >= 256)
        pixelCycle = 0;                         //  Loop the cycle back to the begining   
}
