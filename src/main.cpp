#define MQTT_MAX_PACKET_SIZE 512

extern "C" {
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

#define PREF_APP_KEY "JBLedController"
#define PREF_INITIALIZED_KEY "initialized"

Preferences _preferences;
//WiFiClient _wifiClient;
//PubSubClient _mqttClient(_wifiClient);

AsyncMqttClient _mqttClient;
TimerHandle_t _mqttReconnectTimer;
TimerHandle_t _wifiReconnectTimer;

LedController _ledController(&_preferences);

unsigned long nextExecution;

void mqttAutoDiscovery()
{
    Serial.println("Preparing MQTT auto discovery for Homeassistant");
    StaticJsonDocument<512> jsonDoc;

    auto topic = DeviceUtils::GetBaseTopic(&_preferences);
    auto deviceId = DeviceUtils::GetDeviceId(&_preferences);

    jsonDoc["~"] = topic;
    jsonDoc["name"] = deviceId;
    jsonDoc["unique_id"] = deviceId;
    jsonDoc["cmd_t"] = F("~/set");
    jsonDoc["stat_t"] = F("~/state");
    jsonDoc["schema"] = F("json");
    jsonDoc["color_mode"] = true;
    jsonDoc["brightness"] = true;
    auto supportedColorModesArray =jsonDoc.createNestedArray(F("supported_color_modes"));
    supportedColorModesArray.add(F("rgbw"));
    //supportedColorModesArray.add(F("rgb"));
    //jsonDoc["optimistic"] = false;

    auto discoveryTopic = DeviceUtils::GetHomeAssistantDiscoveryTopic(&_preferences);

    // Serial.println(discoveryTopic);
    // Serial.println(F("Autodiscovery payload: "));
    // serializeJsonPretty(jsonDoc, Serial);
    // Serial.println(F(" "));

    char buffer[512];
    size_t numberOfBytes = serializeJson(jsonDoc, buffer);

    _mqttClient.publish(discoveryTopic.c_str(), 0, false, buffer, numberOfBytes);
}

void sendStateUpdate()
{
    Serial.println("Sending a light state update to MQTT");
    StaticJsonDocument<512> jsonDoc;
    JsonObject jsonObject = jsonDoc.to<JsonObject>();

    auto state = _ledController.getState();
    
    if (state->lightOn)
        jsonDoc[JSON_STATE_KEY] = F("ON");
    else
        jsonDoc[JSON_STATE_KEY] = F("OFF");

    jsonDoc[JSON_BRIGHTNESS_KEY] = state->brightness;
    jsonDoc[JSON_COLOR_MODE_KEY] = F("rgbw");
    jsonDoc[JSON_COLOR_KEY][JSON_RED_KEY] = state->red;
    jsonDoc[JSON_COLOR_KEY][JSON_GREEN_KEY] = state->green;
    jsonDoc[JSON_COLOR_KEY][JSON_BLUE_KEY] = state->blue;
    jsonDoc[JSON_COLOR_KEY][JSON_WHITE_KEY] = state->white;

    char buffer[512];
    size_t numberOfBytes = serializeJson(jsonDoc, buffer);

    Serial.println("Current light state is: ");
    serializeJsonPretty(jsonDoc, Serial);

    auto topic = DeviceUtils::GetStateTopic(&_preferences).c_str();

    Serial.println(topic);

    _mqttClient.publish(topic, 0, true, buffer, numberOfBytes);
    //_mqttClient.publish(topic, buffer, numberOfBytes);
}

void connectToWifi() 
{
    Serial.println("Connecting to Wi-Fi...");
    WiFi.begin(SSID_NAME, SSID_PASSWORD);
}

void connectToMqtt()
{
    Serial.println("connecting to MQTT...");
    _mqttClient.connect();
}

void wifiEvent(WiFiEvent_t event)
{
    //Serial.printf("[WiFi-event] event: %d\n", event);
    
    switch(event) 
    {
        case SYSTEM_EVENT_STA_GOT_IP:
            Serial.println("WiFi connected");
            Serial.println("IP address: ");
            Serial.println(WiFi.localIP());
            connectToMqtt();
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            Serial.println("WiFi lost connection");
            xTimerStop(_mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
            xTimerStart(_wifiReconnectTimer, 0);
            break;
    }  
}

void onMqttConnected(bool sessionPresent) {
    Serial.println("Connected to MQTT.");
    // Serial.print("Session present: ");
    // Serial.println(sessionPresent);

    _mqttClient.subscribe(DeviceUtils::GetCommandTopic(&_preferences).c_str(), 0);
    mqttAutoDiscovery();

//   uint16_t packetIdSub = _mqttClient.subscribe("test/lol", 2);
//   Serial.print("Subscribing at QoS 2, packetId: ");
//   Serial.println(packetIdSub);
//   _mqttClient.publish("test/lol", 0, true, "test 1");
//   Serial.println("Publishing at QoS 0");
//   uint16_t packetIdPub1 = _mqttClient.publish("test/lol", 1, true, "test 2");
//   Serial.print("Publishing at QoS 1, packetId: ");
//   Serial.println(packetIdPub1);
//   uint16_t packetIdPub2 = _mqttClient.publish("test/lol", 2, true, "test 3");
//   Serial.print("Publishing at QoS 2, packetId: ");
//   Serial.println(packetIdPub2);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) 
{
    Serial.println("Disconnected from MQTT.");

    if (WiFi.isConnected()) {
        xTimerStart(_mqttReconnectTimer, 0);
    }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) 
{
    // Serial.println("Subscribe acknowledged.");
    // Serial.print("  packetId: ");
    // Serial.println(packetId);
    // Serial.print("  qos: ");
    // Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId) 
{
    Serial.println("Unsubscribe acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) 
{
    Serial.println("MQTT message received");

    StaticJsonDocument<512> jsonDoc;
    deserializeJson(jsonDoc, payload, len);

#if DEBUG
    serializeJsonPretty(jsonDoc, Serial);
#endif


    if (strcmp(topic, DeviceUtils::GetCommandTopic(&_preferences).c_str()) == 0)
    {
        LightStateUpdate stateUpdate;

        if (jsonDoc.containsKey(JSON_STATE_KEY)){
            stateUpdate.lightOnPresent = true;

            auto state = jsonDoc[JSON_STATE_KEY];
            if (strcmp(state, "ON") == 0)
                stateUpdate.lightOn = true;
            else if (strcmp(state, "OFF") == 0)
                stateUpdate.lightOn = false;
        }

        if (jsonDoc.containsKey(JSON_BRIGHTNESS_KEY))
        {
            stateUpdate.brightnessPresent = true;
            stateUpdate.brightness = jsonDoc[JSON_BRIGHTNESS_KEY];
        }

        if (jsonDoc.containsKey(JSON_COLOR_KEY))
        {
            JsonVariant colorVariant = jsonDoc[JSON_COLOR_KEY];

            if (colorVariant.containsKey(JSON_RED_KEY)){
                stateUpdate.redPresent = true;
                stateUpdate.red = colorVariant[JSON_RED_KEY];
            }

            if (colorVariant.containsKey(JSON_GREEN_KEY)){
                stateUpdate.greenPresent = true;
                stateUpdate.green = colorVariant[JSON_GREEN_KEY];
            }

            if (colorVariant.containsKey(JSON_BLUE_KEY)){
                stateUpdate.bluePresent = true;
                stateUpdate.blue = colorVariant[JSON_BLUE_KEY];
            }

            if (colorVariant.containsKey(JSON_WHITE_KEY)){
                stateUpdate.whitePresent = true;
                stateUpdate.white = colorVariant[JSON_WHITE_KEY];
            }
        }

        _ledController.setState(stateUpdate);
    }

    delay(1000);
    sendStateUpdate();
}

void onMqttPublish(uint16_t packetId) 
{
    Serial.println("Publish acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
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
        Serial.println("not the first boot");
    }
    else
    {
        // this seems to be the first boot
        Serial.println("first boot");

        // generate a device id..
        auto deviceId = DeviceUtils::GenerateDeviceId();

        // and store it 
        _preferences.putString(PREF_DEVICE_NAME_KEY, deviceId);

#if DEBUG
        Serial.println(deviceId);
#endif

        // initialization is done, 
        // set the flag in _preferences to show, that we are properly initialized
        _preferences.putBool(PREF_INITIALIZED_KEY, true);
    }
}

void initWifi()
{
    Serial.println("Init: WIFI");

    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID_NAME, SSID_PASSWORD);

#if DEBUG
    Serial.println();
    Serial.print("Connecting to: ");
    Serial.println(SSID_NAME);
#endif

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    randomSeed(micros());

#if DEBUG
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
#endif
    Serial.println("End-Init: WIFI");
}

void setup()
{
    // setup the serial port
    Serial.begin(115200);

    // safety delay
    delay(1000);

    init_preferences();
    initConfig();
    //initWifi();

    _mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
    _wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

    WiFi.onEvent(wifiEvent);

    _mqttClient.onConnect(onMqttConnected);
    _mqttClient.onDisconnect(onMqttDisconnect);
    _mqttClient.onSubscribe(onMqttSubscribe);
    _mqttClient.onUnsubscribe(onMqttUnsubscribe);
    _mqttClient.onMessage(onMqttMessage);
    _mqttClient.onPublish(onMqttPublish);
    _mqttClient.setServer(MQTT_BROKER, MQTT_PORT);

    connectToWifi();
    _ledController.setup();
}

void loop()
{
    _ledController.loop();
}