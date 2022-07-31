#ifndef __DEVICEUTILS_H__
#define __DEVICEUTILS_H__

#include <Arduino.h>
#include "Preferences.h"

/**
 * @brief Utility methods for device specific function
 */
class DeviceUtils
{
private:
    Preferences * _preferences;
public:

    DeviceUtils(Preferences* preferences)
    {
        _preferences = preferences;
    }

    /**
     * @brief Get a unique identifier for a ESP-Device, it will be based on the devices Mac-Address
     *
     * @return Device identifier
     */
    static String GenerateDeviceId()
    {
        // read the mac address from device
        unsigned char mac_base[6] = {0};
        esp_efuse_mac_get_default(mac_base);
        esp_read_mac(mac_base, ESP_MAC_WIFI_STA);

        // format the mac address in HEX
        char macAddress[6];
        sprintf(macAddress, "%02X%02X%02X", mac_base[3], mac_base[4], mac_base[5]);

        // compose the device name
        String deviceIdString = "LEDCont";
        deviceIdString += macAddress[0];
        deviceIdString += macAddress[1];
        deviceIdString += macAddress[2];
        deviceIdString += macAddress[3];
        deviceIdString += macAddress[4];
        deviceIdString += macAddress[5];

        return deviceIdString;
    }

    String GetDeviceId()
    {
        return _preferences->getString(PREF_DEVICE_NAME_KEY);
    }

    String GetBaseTopic()
    {
        auto topic = String("homeassistant/light/");
        topic += GetDeviceId();

        return topic;
    }

    String GetStateTopic()
    {
        return GetBaseTopic() + "/state";
    }

    String GetCommandTopic()
    {
        return GetBaseTopic() + "/set";
    }

    String GetHomeAssistantDiscoveryTopic()
    {
        return GetBaseTopic() + "/config";
    }
};

#endif // __DEVICEUTILS_H__