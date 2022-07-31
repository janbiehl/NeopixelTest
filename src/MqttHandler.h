#ifndef __MQTTHANDLER_H__
#define __MQTTHANDLER_H__

#include "AsyncMqttClient.h"
#include "DeviceUtils.h"
#include "ArduinoJson.h"
#include "LedController.h"

#define HA_DISCOVERY_BUFFER_SIZE 512
#define LIGHT_STATE_BUFFER_SIZE 512
#define MESSAGE_BUFFER_SIZE 512

class MqttHandler
{
private:
    AsyncMqttClient _mqttClient;
    DeviceUtils* _deviceUtils;
    TimerHandle_t _reconnectTimer;
    LedController* _lightControllerOne;
    bool _shouldConnect;

public:
static void Test();
    MqttHandler(DeviceUtils* deviceUtils, LedController* lightControllerOne);
    /**
     * @brief Initialize the MQTT handler
     * 
     */
    void setup();
    /**
     * @brief Connect to the MQTT broker
     * 
     */
    void connect();
    /**
     * @brief Disconnect from the MQTT broker
     * 
     */
    void disconnect();
    /**
     * @brief Publishes a message that will be recognized by HomeAssistant, 
     * so that the LED-Controller will be recognized by it.
     * In HomeAssistant no setup will be required
     * 
     */
    void sendHomeAssistantAutoDiscoveryMessage();
    /**
     * @brief Publishes a message with the current state from LEDController
     * 
     * @param lightState The state to send
     */
    void sendLightStateUpdate(LightState* lightState);

    void startReconnectTimer(uint16_t ticksToWait);
    void stopReconnectTimer(uint16_t ticksToWait);

private:

    LightStateUpdate parseLightControllerMessage(char* payload, size_t length, size_t index, size_t total);

    /**
     * @brief Callback that will be invoked, when MQTT is connected
     */
    void onMqttConnected(bool sessionPresent);
    /**
     * @brief Callback that will be invoked when MQTT has disconnected
     */
    void onMqttDisconnected(AsyncMqttClientDisconnectReason disconnectReason);
    /**
     * @brief Callback that will be invoked when MQTT has subscribed to a topic
     */
    void onMqttSubscribe(uint16_t packetId, uint8_t qualityOfService);
    /**
     * @brief Callback that will be invoked when a topic was unsubscribed
     */
    void onMqttUnsubscribe(uint16_t packetId);
    /**
     * @brief Callback that will be invoked when a message was received at a subscribed topic
     * 
     * @param topic The topic the message was received at
     * @param payload The data for this message
     * @param properties 
     * @param len 
     * @param index 
     * @param total 
     */
    void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
    /**
     * @brief Callback that will be invoked when a message was published
     * 
     * @param packetId 
     */
    void onMqttPublish(uint16_t packetId);
};

#endif // __MQTTHANDLER_H__
