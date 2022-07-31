#include "MqttHandler.h"

// Constructor
MqttHandler::MqttHandler(DeviceUtils* deviceUtils, LedController* lightControllerOne)
{
    _deviceUtils = deviceUtils;
    _lightControllerOne = lightControllerOne;

    auto onTimer = [](TimerHandle_t hTmr) {
        MqttHandler* mm = static_cast<MqttHandler*>(pvTimerGetTimerID(hTmr)); // Retrieve the pointer to class
        assert(mm); // Sanity check
        mm->connect();
    };

    _reconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, static_cast<void*>(this), onTimer);
}

void MqttHandler::setup()
{
    // Register callbacks
    _mqttClient.onConnect(
        [&](bool sessionPresent)
        {
            onMqttConnected(sessionPresent);
        });

    _mqttClient.onDisconnect(
        [&](AsyncMqttClientDisconnectReason disconnectReason)
        {
            onMqttDisconnected(disconnectReason);
        });

    _mqttClient.onSubscribe(
        [&](uint16_t packetId, uint8_t qualityOfService)
        {
            onMqttSubscribe(packetId, qualityOfService);
        });

    _mqttClient.onUnsubscribe(
        [&](uint16_t packetId)
        {
            onMqttUnsubscribe(packetId);
        });

    _mqttClient.onMessage(
        [&](char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
        {
            onMqttMessage(topic, payload, properties, len, index, total);
        });

    _mqttClient.onPublish(
        [&](uint16_t packetId)
        {
            onMqttPublish(packetId);
        });

    _mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
}

void MqttHandler::connect()
{
    _shouldConnect = true;
    _mqttClient.connect();
}

void MqttHandler::disconnect()
{
    if (!_shouldConnect)
        return;

    _shouldConnect = false;
    _mqttClient.disconnect();
}

void MqttHandler::sendHomeAssistantAutoDiscoveryMessage()
{
    Serial.println(F("sending MQTT auto discovery for HomeAssistant"));

    StaticJsonDocument<HA_DISCOVERY_BUFFER_SIZE> jsonDocument;
    String baseTopic = _deviceUtils->GetBaseTopic();
    String deviceId = _deviceUtils->GetDeviceId();

    jsonDocument["~"] = baseTopic;
    jsonDocument["name"] = deviceId;
    jsonDocument["unique_id"] = deviceId;
    jsonDocument["cmd_t"] = F("~/set");
    jsonDocument["stat_t"] = F("~/state");
    jsonDocument["schema"] = F("json");
    jsonDocument["color_mode"] = true;
    jsonDocument["brightness"] = true;
    auto supportedColorModesArray = jsonDocument.createNestedArray(F("supported_color_modes"));
    supportedColorModesArray.add(F("rgbw"));

    jsonDocument["effect"] = true;
    auto effectListArray = jsonDocument.createNestedArray(F("effect_list"));
    effectListArray.add(F("solid"));
    effectListArray.add(F("rainbow"));

    String discoveryTopic = _deviceUtils->GetHomeAssistantDiscoveryTopic();

    char buffer[512];
    size_t numberOfBytes = serializeJson(jsonDocument, buffer);

#if DEBUG_MQTT

    serializeJsonPretty(jsonDocument, Serial);
    Serial.println(F(""));

#endif

    _mqttClient.publish(discoveryTopic.c_str(), 0, false, buffer, numberOfBytes);
}

void MqttHandler::sendLightStateUpdate(LightState* lightState)
{
    Serial.println(F("Sending a light state update to MQTT"));
    StaticJsonDocument<LIGHT_STATE_BUFFER_SIZE> jsonDoc;
    JsonObject jsonObject = jsonDoc.to<JsonObject>();

    if (lightState->lightOn)
        jsonDoc[JSON_STATE_KEY] = F("ON");
    else
        jsonDoc[JSON_STATE_KEY] = F("OFF");

    jsonDoc[JSON_BRIGHTNESS_KEY] = lightState->brightness;
    jsonDoc[JSON_COLOR_MODE_KEY] = F("rgbw");
    jsonDoc[JSON_COLOR_KEY][JSON_RED_KEY] = lightState->red;
    jsonDoc[JSON_COLOR_KEY][JSON_GREEN_KEY] = lightState->green;
    jsonDoc[JSON_COLOR_KEY][JSON_BLUE_KEY] = lightState->blue;
    jsonDoc[JSON_COLOR_KEY][JSON_WHITE_KEY] = lightState->white;

    if (lightState->lightEffect == LightEffect::solid)
    {
        jsonDoc[JSON_EFFECT_KEY] = "solid";
    }
    else if (lightState->lightEffect == LightEffect::rainbow)
    {
        jsonDoc[JSON_EFFECT_KEY] = "rainbow";
    }
    else
    {
        // Solid as fallback
        jsonDoc[JSON_EFFECT_KEY] = "solid";
    }

    char buffer[LIGHT_STATE_BUFFER_SIZE];
    size_t numberOfBytes = serializeJson(jsonDoc, buffer);

    String topic = _deviceUtils->GetStateTopic();

    Serial.printf("Sending the state update to: '%s'", topic.c_str());

#if DEBUG_MQTT

    Serial.println(F("Light state for MQTT: "));
    serializeJsonPretty(jsonDoc, Serial);
    Serial.println(F(""));

#endif

    // TODO: Why is this required to be retained? I dont get it right now.
    _mqttClient.publish(topic.c_str(), 0, true, buffer, numberOfBytes);

}

void MqttHandler::startReconnectTimer(uint16_t ticksToWait)
{
    xTimerStart(_reconnectTimer, ticksToWait);
}

void MqttHandler::stopReconnectTimer(uint16_t ticksToWait)
{
    xTimerStop(_reconnectTimer, ticksToWait);
}

LightStateUpdate MqttHandler::parseLightControllerMessage(char* payload, size_t length, size_t index, size_t total)
{
    StaticJsonDocument<MESSAGE_BUFFER_SIZE> jsonDoc;
    deserializeJson(jsonDoc, payload, length);

#if DEBUG_MQTT
    serializeJsonPretty(jsonDoc, Serial);
    Serial.println(F(""));
#endif

    LightStateUpdate stateUpdate = LightStateUpdate();

    if (jsonDoc.containsKey(JSON_STATE_KEY))
    {
#if DEBUG_MQTT
        Serial.println(F("Message contains state information"));
#endif
        stateUpdate.lightOnPresent = true;

        auto state = jsonDoc[JSON_STATE_KEY];
        if (strcmp(state, "ON") == 0)
            stateUpdate.lightOn = true;
        else if (strcmp(state, "OFF") == 0)
            stateUpdate.lightOn = false;
    }

    if (jsonDoc.containsKey(JSON_BRIGHTNESS_KEY))
    {
#if DEBUG_MQTT
        Serial.println(F("Message contains brightness information"));
#endif
        stateUpdate.brightnessPresent = true;
        stateUpdate.brightness = jsonDoc[JSON_BRIGHTNESS_KEY];
    }

    if (jsonDoc.containsKey(JSON_COLOR_KEY))
    {
#if DEBUG_MQTT
        Serial.println(F("Message contains color information"));
#endif
        JsonVariant colorVariant = jsonDoc[JSON_COLOR_KEY];

        if (colorVariant.containsKey(JSON_RED_KEY))
        {
#if DEBUG_MQTT
            Serial.println(F("Message contains color red information"));
#endif
            stateUpdate.redPresent = true;
            stateUpdate.red = colorVariant[JSON_RED_KEY];
        }

        if (colorVariant.containsKey(JSON_GREEN_KEY))
        {
#if DEBUG_MQTT
            Serial.println(F("Message contains color green information"));
#endif
            stateUpdate.greenPresent = true;
            stateUpdate.green = colorVariant[JSON_GREEN_KEY];
        }

        if (colorVariant.containsKey(JSON_BLUE_KEY))
        {
#if DEBUG_MQTT
            Serial.println(F("Message contains color blue information"));
#endif
            stateUpdate.bluePresent = true;
            stateUpdate.blue = colorVariant[JSON_BLUE_KEY];
        }

        if (colorVariant.containsKey(JSON_WHITE_KEY))
        {
#if DEBUG_MQTT
            Serial.println(F("Message contains color white information"));
#endif
            stateUpdate.whitePresent = true;
            stateUpdate.white = colorVariant[JSON_WHITE_KEY];
        }
    }

    if (jsonDoc.containsKey(JSON_EFFECT_KEY))
    {
#if DEBUG_MQTT
        Serial.println(F("Message contains effect information"));
#endif
        stateUpdate.lightEffectPresent = true;

        String effectString = jsonDoc[JSON_EFFECT_KEY].as<String>();

        if (effectString.equals("solid"))
        {
            stateUpdate.lightEffect = LightEffect::solid;
        }
        else if (effectString.equals("rainbow"))
        {
            stateUpdate.lightEffect = LightEffect::rainbow;
        }
        else
        {
            stateUpdate.lightEffect = unknown;
            stateUpdate.lightEffectPresent = false;
            Serial.printf("light effect: '%s' is not supported\n", effectString.c_str());
        }
    }

    return stateUpdate;
}

void MqttHandler::onMqttConnected(bool sessionPresent)
{
    Serial.println(F("Connected to MQTT."));

#if DEBUG_MQTT
    Serial.println(F("Subscribing for light command topic"));
#endif

    _mqttClient.subscribe(_deviceUtils->GetCommandTopic().c_str(), 0);

    // TODO: Check if 200 is enough bevore it was 500
    delay(200);

    sendHomeAssistantAutoDiscoveryMessage();

    // TODO: Check if 200 is enough bevore it was 500
    delay(200);

    // TODO: Send light state update
    //sendStateUpdate();
}

void MqttHandler::onMqttDisconnected(AsyncMqttClientDisconnectReason disconnectReason)
{
    Serial.println(F("Disconnected from MQTT."));

    if (_shouldConnect)
    {
        // TODO: We have to check this
        xTimerStart(_reconnectTimer, 0);
    }
}

void MqttHandler::onMqttSubscribe(uint16_t packetId, uint8_t qualityOfService)
{
#if DEBUG_MQTT
    Serial.println(F("Subscribe acknowledged."));
    Serial.print(F("  packetId: "));
    Serial.println(packetId);
    Serial.print(F("  qos: "));
    Serial.println(qos);
#endif
}

void MqttHandler::onMqttUnsubscribe(uint16_t packetId)
{
#if DEBUG_MQTT
    Serial.println(F("Unsubscribe acknowledged."));
    Serial.print(F("  packetId: "));
    Serial.println(packetId);
#endif
}

void MqttHandler::onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
    Serial.printf("MQTT message at topic: '%s' received\n", topic);

    if (strcmp(topic, _deviceUtils->GetCommandTopic().c_str()) == 0)
    {
#if DEBUG_MQTT
        Serial.printf("\nthere was a mqtt message at '%s'\n", commandTopic.c_str());
#endif

        auto newLightState = parseLightControllerMessage(payload, len, index, total);
        _lightControllerOne->setState(newLightState);
    }

}

void MqttHandler::onMqttPublish(uint16_t packetId)
{
#if DEBUG_MQTT
    Serial.println(F("Publish acknowledged."));
    Serial.print(F("  packetId: "));
    Serial.println(packetId);
#endif
}
