// Copyright (C) 2024 C-E

#include "Configuration.h"
#include "MessageOutput.h"
#include "defaults.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

CONFIG_T config;

void ConfigurationClass::init()
{
    memset(&config, 0x0, sizeof(config));
    config.Cfg.Version = CONFIG_VERSION;
    config.AutoLimit.MaxLeistung = AUTOLIMIT_MAXLEISTUNG;
}

bool ConfigurationClass::write()
{
    File f = LittleFS.open(CONFIG_FILENAME, "w");
    if (!f) {
        return false;
    }
    config.Cfg.SaveCount++;

    DynamicJsonDocument doc(JSON_BUFFER_SIZE);

    JsonObject cfg = doc.createNestedObject("cfg");
    cfg["version"] = config.Cfg.Version;
    cfg["save_count"] = config.Cfg.SaveCount;

    JsonObject wifi = doc.createNestedObject("wifi");
    wifi["ssid"] = config.WiFi.Ssid;
    wifi["password"] = config.WiFi.Password;
    wifi["ip"] = IPAddress(config.WiFi.Ip).toString();
    wifi["netmask"] = IPAddress(config.WiFi.Netmask).toString();
    wifi["gateway"] = IPAddress(config.WiFi.Gateway).toString();
    wifi["dns1"] = IPAddress(config.WiFi.Dns1).toString();
    wifi["dns2"] = IPAddress(config.WiFi.Dns2).toString();
    wifi["dhcp"] = config.WiFi.Dhcp;
    wifi["hostname"] = config.WiFi.Hostname;
    wifi["aptimeout"] = config.WiFi.ApTimeout;

    JsonObject mdns = doc.createNestedObject("mdns");
    mdns["enabled"] = config.Mdns.Enabled;

    JsonObject ntp = doc.createNestedObject("ntp");
    ntp["server"] = config.Ntp.Server;
    ntp["timezone"] = config.Ntp.Timezone;
    ntp["timezone_descr"] = config.Ntp.TimezoneDescr;
    ntp["latitude"] = config.Ntp.Latitude;
    ntp["longitude"] = config.Ntp.Longitude;

    JsonObject dtu = doc.createNestedObject("dtu");
    dtu["serial"] = config.Dtu.Serial;
    dtu["poll_interval"] = config.Dtu.PollInterval;
    dtu["nrf_pa_level"] = config.Dtu.Nrf.PaLevel;
    dtu["cmt_pa_level"] = config.Dtu.Cmt.PaLevel;
    dtu["cmt_frequency"] = config.Dtu.Cmt.Frequency;

    JsonObject autolimit = doc.createNestedObject("autolimit");
    autolimit["maxleistung"] = config.AutoLimit.MaxLeistung;

    JsonObject security = doc.createNestedObject("security");
    security["password"] = config.Security.Password;
    security["allow_readonly"] = config.Security.AllowReadonly;

    JsonObject device = doc.createNestedObject("device");
    device["pinmapping"] = config.Dev_PinMapping;

    JsonObject display = device.createNestedObject("display");
    display["powersafe"] = config.Display.PowerSafe;
    display["screensaver"] = config.Display.ScreenSaver;
    display["rotation"] = config.Display.Rotation;
    display["contrast"] = config.Display.Contrast;
    display["language"] = config.Display.Language;
    display["diagram_duration"] = config.Display.DiagramDuration;

    JsonArray leds = device.createNestedArray("led");
    for (uint8_t i = 0; i < PINMAPPING_LED_COUNT; i++) {
        JsonObject led = leds.createNestedObject();
        led["brightness"] = config.Led_Single[i].Brightness;
    }

    JsonArray inverters = doc.createNestedArray("inverters");
    for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
        JsonObject inv = inverters.createNestedObject();
        inv["serial"] = config.Inverter[i].Serial;
        inv["name"] = config.Inverter[i].Name;
        inv["order"] = config.Inverter[i].Order;
        inv["poll_enable"] = config.Inverter[i].Poll_Enable;
        inv["poll_enable_night"] = config.Inverter[i].Poll_Enable_Night;
        inv["command_enable"] = config.Inverter[i].Command_Enable;
        inv["command_enable_night"] = config.Inverter[i].Command_Enable_Night;
        inv["reachable_threshold"] = config.Inverter[i].ReachableThreshold;
        inv["zero_runtime"] = config.Inverter[i].ZeroRuntimeDataIfUnrechable;
        inv["zero_day"] = config.Inverter[i].ZeroYieldDayOnMidnight;
        inv["yieldday_correction"] = config.Inverter[i].YieldDayCorrection;

        JsonArray channel = inv.createNestedArray("channel");
        for (uint8_t c = 0; c < INV_MAX_CHAN_COUNT; c++) {
            JsonObject chanData = channel.createNestedObject();
            chanData["name"] = config.Inverter[i].channel[c].Name;
            chanData["max_power"] = config.Inverter[i].channel[c].MaxChannelPower;
            chanData["yield_total_offset"] = config.Inverter[i].channel[c].YieldTotalOffset;
        }
    }

    // Serialize JSON to file
    if (serializeJson(doc, f) == 0) {
        MessageOutput.println("Failed to write file");
        return false;
    }

    f.close();
    return true;
}

bool ConfigurationClass::read()
{
    File f = LittleFS.open(CONFIG_FILENAME, "r", false);

    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    // Deserialize the JSON document
    const DeserializationError error = deserializeJson(doc, f);
    if (error) {
        MessageOutput.println("Failed to read file, using default configuration");
    }
    // Inhalt von config.json ausgeben
    //Serial.println("=== config.json start ===");
    //while (f.available()) {
    //    Serial.write(f.read());
    //}
    //Serial.println("\n=== config.json end ===");
    //f.close();

    JsonObject cfg = doc["cfg"];
    config.Cfg.Version = cfg["version"] | CONFIG_VERSION;
    config.Cfg.SaveCount = cfg["save_count"] | 0;

    JsonObject wifi = doc["wifi"];
    strlcpy(config.WiFi.Ssid, wifi["ssid"] | WIFI_SSID, sizeof(config.WiFi.Ssid));
    strlcpy(config.WiFi.Password, wifi["password"] | WIFI_PASSWORD, sizeof(config.WiFi.Password));
    strlcpy(config.WiFi.Hostname, wifi["hostname"] | APP_HOSTNAME, sizeof(config.WiFi.Hostname));

    IPAddress wifi_ip;
    wifi_ip.fromString(wifi["ip"] | "");
    config.WiFi.Ip[0] = wifi_ip[0];
    config.WiFi.Ip[1] = wifi_ip[1];
    config.WiFi.Ip[2] = wifi_ip[2];
    config.WiFi.Ip[3] = wifi_ip[3];

    IPAddress wifi_netmask;
    wifi_netmask.fromString(wifi["netmask"] | "");
    config.WiFi.Netmask[0] = wifi_netmask[0];
    config.WiFi.Netmask[1] = wifi_netmask[1];
    config.WiFi.Netmask[2] = wifi_netmask[2];
    config.WiFi.Netmask[3] = wifi_netmask[3];

    IPAddress wifi_gateway;
    wifi_gateway.fromString(wifi["gateway"] | "");
    config.WiFi.Gateway[0] = wifi_gateway[0];
    config.WiFi.Gateway[1] = wifi_gateway[1];
    config.WiFi.Gateway[2] = wifi_gateway[2];
    config.WiFi.Gateway[3] = wifi_gateway[3];

    IPAddress wifi_dns1;
    wifi_dns1.fromString(wifi["dns1"] | "");
    config.WiFi.Dns1[0] = wifi_dns1[0];
    config.WiFi.Dns1[1] = wifi_dns1[1];
    config.WiFi.Dns1[2] = wifi_dns1[2];
    config.WiFi.Dns1[3] = wifi_dns1[3];

    IPAddress wifi_dns2;
    wifi_dns2.fromString(wifi["dns2"] | "");
    config.WiFi.Dns2[0] = wifi_dns2[0];
    config.WiFi.Dns2[1] = wifi_dns2[1];
    config.WiFi.Dns2[2] = wifi_dns2[2];
    config.WiFi.Dns2[3] = wifi_dns2[3];

    config.WiFi.Dhcp = wifi["dhcp"] | WIFI_DHCP;
    config.WiFi.ApTimeout = wifi["aptimeout"] | ACCESS_POINT_TIMEOUT;

    JsonObject mdns = doc["mdns"];
    config.Mdns.Enabled = mdns["enabled"] | MDNS_ENABLED;

    JsonObject ntp = doc["ntp"];
    strlcpy(config.Ntp.Server, ntp["server"] | NTP_SERVER, sizeof(config.Ntp.Server));
    strlcpy(config.Ntp.Timezone, ntp["timezone"] | NTP_TIMEZONE, sizeof(config.Ntp.Timezone));
    strlcpy(config.Ntp.TimezoneDescr, ntp["timezone_descr"] | NTP_TIMEZONEDESCR, sizeof(config.Ntp.TimezoneDescr));
    config.Ntp.Latitude = ntp["latitude"] | NTP_LATITUDE;
    config.Ntp.Longitude = ntp["longitude"] | NTP_LONGITUDE;

    JsonObject dtu = doc["dtu"];
    config.Dtu.Serial = dtu["serial"] | DTU_SERIAL;
    config.Dtu.PollInterval = dtu["poll_interval"] | DTU_POLL_INTERVAL;
    config.Dtu.Nrf.PaLevel = dtu["nrf_pa_level"] | DTU_NRF_PA_LEVEL;
    config.Dtu.Cmt.PaLevel = dtu["cmt_pa_level"] | DTU_CMT_PA_LEVEL;
    config.Dtu.Cmt.Frequency = dtu["cmt_frequency"] | DTU_CMT_FREQUENCY;

    JsonObject autolimit = doc["autolimit"];
    config.AutoLimit.MaxLeistung = autolimit["automaxleistung"] | AUTOLIMIT_MAXLEISTUNG;

    JsonObject security = doc["security"];
    strlcpy(config.Security.Password, security["password"] | ACCESS_POINT_PASSWORD, sizeof(config.Security.Password));
    config.Security.AllowReadonly = security["allow_readonly"] | SECURITY_ALLOW_READONLY;

    JsonObject device = doc["device"];
    strlcpy(config.Dev_PinMapping, device["pinmapping"] | DEV_PINMAPPING, sizeof(config.Dev_PinMapping));

    JsonObject display = device["display"];
    config.Display.PowerSafe = display["powersafe"] | DISPLAY_POWERSAFE;
    config.Display.ScreenSaver = display["screensaver"] | DISPLAY_SCREENSAVER;
    config.Display.Rotation = display["rotation"] | DISPLAY_ROTATION;
    config.Display.Contrast = display["contrast"] | DISPLAY_CONTRAST;
    config.Display.Language = display["language"] | DISPLAY_LANGUAGE;
    config.Display.DiagramDuration = display["diagram_duration"] | DISPLAY_DIAGRAM_DURATION;

    JsonArray leds = device["led"];
    for (uint8_t i = 0; i < PINMAPPING_LED_COUNT; i++) {
        JsonObject led = leds[i].as<JsonObject>();
        config.Led_Single[i].Brightness = led["brightness"] | LED_BRIGHTNESS;
    }

    JsonArray inverters = doc["inverters"];
    for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
        JsonObject inv = inverters[i].as<JsonObject>();
        config.Inverter[i].Serial = inv["serial"] | 0ULL;
        strlcpy(config.Inverter[i].Name, inv["name"] | "", sizeof(config.Inverter[i].Name));
        config.Inverter[i].Order = inv["order"] | 0;

        config.Inverter[i].Poll_Enable = inv["poll_enable"] | true;
        config.Inverter[i].Poll_Enable_Night = inv["poll_enable_night"] | true;
        config.Inverter[i].Command_Enable = inv["command_enable"] | true;
        config.Inverter[i].Command_Enable_Night = inv["command_enable_night"] | true;
        config.Inverter[i].ReachableThreshold = inv["reachable_threshold"] | REACHABLE_THRESHOLD;
        config.Inverter[i].ZeroRuntimeDataIfUnrechable = inv["zero_runtime"] | false;
        config.Inverter[i].ZeroYieldDayOnMidnight = inv["zero_day"] | false;
        config.Inverter[i].YieldDayCorrection = inv["yieldday_correction"] | false;

        JsonArray channel = inv["channel"];
        for (uint8_t c = 0; c < INV_MAX_CHAN_COUNT; c++) {
            config.Inverter[i].channel[c].MaxChannelPower = channel[c]["max_power"] | 0;
            config.Inverter[i].channel[c].YieldTotalOffset = channel[c]["yield_total_offset"] | 0.0f;
            strlcpy(config.Inverter[i].channel[c].Name, channel[c]["name"] | "", sizeof(config.Inverter[i].channel[c].Name));
        }
    }

    f.close();
    return true;
}

CONFIG_T& ConfigurationClass::get()
{
    return config;
}

INVERTER_CONFIG_T* ConfigurationClass::getFreeInverterSlot()
{
    for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
        if (config.Inverter[i].Serial == 0) {
            return &config.Inverter[i];
        }
    }

    return nullptr;
}

INVERTER_CONFIG_T* ConfigurationClass::getInverterConfig(const uint64_t serial)
{
    for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
        if (config.Inverter[i].Serial == serial) {
            return &config.Inverter[i];
        }
    }

    return nullptr;
}

ConfigurationClass Configuration;