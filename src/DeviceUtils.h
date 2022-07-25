#ifndef __DEVICEUTILS_H__
#define __DEVICEUTILS_H__

#include <Arduino.h>
#include "Preferences.h"

/**
 * @brief Utility methods for device specific function 
 */
class DeviceUtils
{    
public:
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
        String deviceIdString = "LEDCont-";
        deviceIdString += macAddress[0];
        deviceIdString += macAddress[1];
        deviceIdString += macAddress[2];
        deviceIdString += macAddress[3];
        deviceIdString += macAddress[4];
        deviceIdString += macAddress[5];

        return deviceIdString;
    }

    static String GetDeviceId(Preferences* _preferences)
    {
        return _preferences->getString(PREF_DEVICE_NAME_KEY);
    }

    static String GetBaseTopic(Preferences* _preferences)
    {
        auto topic = String("homeassistant/light/");
        topic += GetDeviceId(_preferences);

        return topic;
    }
    
    static String GetStateTopic(Preferences* _preferences)
    {
        return GetBaseTopic(_preferences) + "/state";
    }

    static String GetCommandTopic(Preferences* _preferences)
    {
        return GetBaseTopic(_preferences) + "/set";
    }

    static String GetHomeAssistantDiscoveryTopic(Preferences* _preferences)
    {
        return GetBaseTopic(_preferences) + "/config";
    }
};

#endif // __DEVICEUTILS_H__