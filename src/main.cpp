extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
}

#include "Arduino.h"
#include "Preferences.h"
#include "LedController.h"
#include "WiFi.h"
#include "PubSubClient.h"
#include "DeviceUtils.h"
#include "ArduinoJson.h"
#include "AsyncMqttClient.h"
#include "ESPAsyncWebServer.h"
#include "AsyncElegantOTA.h"
#include "MqttHandler.h"

#define PREF_APP_KEY "JBLedController"
#define PREF_INITIALIZED_KEY "initialized"

Preferences _preferences;
TimerHandle_t _wifiReconnectTimer;

DeviceUtils _deviceUtils(&_preferences);
LedController _ledController(&_preferences);
AsyncWebServer _server(80);
MqttHandler _mqttHandler(&_deviceUtils, &_ledController);

void connectToWifi()
{
    Serial.println(F("Connecting to Wi-Fi..."));
    WiFi.begin(SSID_NAME, SSID_PASSWORD);
}

void connectToMqtt()
{
    Serial.println(F("connecting to MQTT..."));
    _mqttHandler.connect();
}

void wifiEvent(WiFiEvent_t event)
{
    switch (event)
    {
    case SYSTEM_EVENT_STA_GOT_IP:
        Serial.println(F("WiFi connected"));
        Serial.println(F("IP address: "));
        Serial.println(WiFi.localIP());

        AsyncElegantOTA.begin(&_server); // Start ElegantOTA
        _server.begin();

        connectToMqtt();
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        Serial.println(F("WiFi lost connection"));
        _mqttHandler.stopReconnectTimer(0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
        xTimerStart(_wifiReconnectTimer, 0);
        break;
    }
}

void init_preferences()
{
    // init the _preferences
    if (!_preferences.begin(PREF_APP_KEY))
    {
        Serial.println(F("_preferences could not be set up"));

        delay(5000);
        esp_restart();
    }
}

void initConfig()
{
    bool isInitialized = _preferences.getBool(PREF_INITIALIZED_KEY, false);

    if (isInitialized)
    {
        // this seems NOT to be the first boot
        Serial.println(F("not the first boot"));

        String deviceId = _preferences.getString(PREF_DEVICE_NAME_KEY);
    }
    else
    {
        // this seems to be the first boot
        Serial.println(F("first boot"));

        // generate a device id..
        String deviceId = _deviceUtils.GenerateDeviceId();

        // and store it
        _preferences.putString(PREF_DEVICE_NAME_KEY, deviceId.c_str());

#if DEBUG
        Serial.println(deviceId.c_str());
#endif

        // initialization is done,
        // set the flag in _preferences to show, that we are properly initialized
        _preferences.putBool(PREF_INITIALIZED_KEY, true);
    }
}

void initWifi()
{
    Serial.println(F("Init: WIFI"));

    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID_NAME, SSID_PASSWORD);

#if DEBUG
    Serial.println();
    Serial.print(F("Connecting to: "));
    Serial.println(SSID_NAME);
#endif

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(F("."));
    }

    randomSeed(micros());

#if DEBUG
    Serial.println(F(""));
    Serial.println(F("WiFi connected"));
    Serial.println(F("IP address: "));
    Serial.println(WiFi.localIP());
#endif
    Serial.println(F("End-Init: WIFI"));
}

void setup()
{
    // setup the serial port
    Serial.begin(115200);

    // safety delay
    delay(500);

    init_preferences();
    initConfig();

    _wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

    WiFi.onEvent(wifiEvent);

    _server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
               { request->send(200, "text/plain", "Hi! I am an LED-Controller \n\n OTA should be enabled for this one at: 'http://<IPAddress>/update'"); });

    connectToWifi();
    _ledController.setup();
}

void loop()
{
    _ledController.loop();
}