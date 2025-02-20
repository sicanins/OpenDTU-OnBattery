// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "Configuration.h"
#include "Datastore.h"
#include "Display_Graphic.h"
#include "InverterSettings.h"
#include "Led_Single.h"
#include "MessageOutput.h"
#include "VeDirectMpptController.h"
#include "Battery.h"
#include "Huawei_can.h"
#include "MqttHandleDtu.h"
#include "MqttHandleHass.h"
#include "MqttHandleVedirectHass.h"
#include "MqttHandlePylontechHass.h"
#include "MqttHandleInverter.h"
#include "MqttHandleInverterTotal.h"
#include "MqttHandleVedirect.h"
#include "MqttHandleHuawei.h"
#include "MqttHandlePowerLimiter.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include "NtpSettings.h"
#include "PinMapping.h"
#include "SunPosition.h"
#include "Utils.h"
#include "WebApi.h"
#include "PowerMeter.h"
#include "PowerLimiter.h"
#include "defaults.h"
#include <Arduino.h>
#include <LittleFS.h>

void setup()
{
    // Initialize serial output
    Serial.begin(SERIAL_BAUDRATE);
#if ARDUINO_USB_CDC_ON_BOOT
    Serial.setTxTimeoutMs(0);
    delay(100);
#else
    while (!Serial)
        yield();
#endif
    MessageOutput.println();
    MessageOutput.println("Starting OpenDTU");

    // Initialize file system
    MessageOutput.print("Initialize FS... ");
    if (!LittleFS.begin(false)) { // Do not format if mount failed
        MessageOutput.print("failed... trying to format...");
        if (!LittleFS.begin(true)) {
            MessageOutput.print("success");
        } else {
            MessageOutput.print("failed");
        }
    } else {
        MessageOutput.println("done");
    }

    // Read configuration values
    MessageOutput.print("Reading configuration... ");
    if (!Configuration.read()) {
        MessageOutput.print("initializing... ");
        Configuration.init();
        if (Configuration.write()) {
            MessageOutput.print("written... ");
        } else {
            MessageOutput.print("failed... ");
        }
    }
    if (Configuration.get().Cfg_Version != CONFIG_VERSION) {
        MessageOutput.print("migrated... ");
        Configuration.migrate();
    }
    CONFIG_T& config = Configuration.get();
    MessageOutput.println("done");

    // Load PinMapping
    MessageOutput.print("Reading PinMapping... ");
    if (PinMapping.init(String(Configuration.get().Dev_PinMapping))) {
        MessageOutput.print("found valid mapping ");
    } else {
        MessageOutput.print("using default config ");
    }
    const PinMapping_t& pin = PinMapping.get();
    MessageOutput.println("done");

    // Initialize WiFi
    MessageOutput.print("Initialize Network... ");
    NetworkSettings.init();
    MessageOutput.println("done");
    NetworkSettings.applyConfig();

    // Initialize NTP
    MessageOutput.print("Initialize NTP... ");
    NtpSettings.init();
    MessageOutput.println("done");

    // Initialize SunPosition
    MessageOutput.print("Initialize SunPosition... ");
    SunPosition.init();
    MessageOutput.println("done");

    // Initialize MqTT
    MessageOutput.print("Initialize MqTT... ");
    MqttSettings.init();
    MqttHandleDtu.init();
    MqttHandleInverter.init();
    MqttHandleInverterTotal.init();
    MqttHandleVedirect.init();
    MqttHandleHass.init();
    MqttHandleVedirectHass.init();
    MqttHandleHuawei.init();
    MqttHandlePowerLimiter.init();
    MessageOutput.println("done");

    // Initialize WebApi
    MessageOutput.print("Initialize WebApi... ");
    WebApi.init();
    MessageOutput.println("done");

    // Initialize Display
    MessageOutput.print("Initialize Display... ");
    Display.init(
        static_cast<DisplayType_t>(pin.display_type),
        pin.display_data,
        pin.display_clk,
        pin.display_cs,
        pin.display_reset);
    Display.setOrientation(config.Display_Rotation);
    Display.enablePowerSafe = config.Display_PowerSafe;
    Display.enableScreensaver = config.Display_ScreenSaver;
    Display.setContrast(config.Display_Contrast);
    Display.setLanguage(config.Display_Language);
    Display.setStartupDisplay();
    MessageOutput.println("done");

    // Initialize Single LEDs
    MessageOutput.print("Initialize LEDs... ");
    LedSingle.init();
    MessageOutput.println("done");

    // Check for default DTU serial
    MessageOutput.print("Check for default DTU serial... ");
    if (config.Dtu_Serial == DTU_SERIAL) {
        MessageOutput.print("generate serial based on ESP chip id: ");
        uint64_t dtuId = Utils::generateDtuSerial();
        MessageOutput.printf("%0x%08x... ",
            ((uint32_t)((dtuId >> 32) & 0xFFFFFFFF)),
            ((uint32_t)(dtuId & 0xFFFFFFFF)));
        config.Dtu_Serial = dtuId;
        Configuration.write();
    }
    MessageOutput.println("done");
    MessageOutput.println("done");

    InverterSettings.init();

    Datastore.init();

    // Initialize ve.direct communication
    MessageOutput.println(F("Initialize ve.direct interfaces... "));
    for (size_t i = 0; i < VICTRON_COUNT; i++)
    {
        if (PinMapping.isValidVictronConfig(i)) {
            MessageOutput.printf("ve.direct #%i rx = %d, tx = %d\r\n", i, pin.victron_rx[i], pin.victron_tx[i]);
            VeDirectMppt[i].init(pin.victron_rx[i], pin.victron_tx[i], i,
                    &MessageOutput, config.Vedirect_VerboseLogging);
            MessageOutput.println(F("done"));
        } else {
            MessageOutput.printf("Invalid pin config #%i\r\n", i);
        }    
    }    
    // Power meter
    PowerMeter.init();

    // Dynamic power limiter
    PowerLimiter.init();

    // Initialize Huawei AC-charger PSU / CAN bus
    MessageOutput.println(F("Initialize Huawei AC charger interface... "));
    if (PinMapping.isValidHuaweiConfig()) {
        MessageOutput.printf("Huawei AC-charger miso = %d, mosi = %d, clk = %d, irq = %d, cs = %d, power_pin = %d\r\n", pin.huawei_miso, pin.huawei_mosi, pin.huawei_clk, pin.huawei_irq, pin.huawei_cs, pin.huawei_power);
        HuaweiCan.init(pin.huawei_miso, pin.huawei_mosi, pin.huawei_clk, pin.huawei_irq, pin.huawei_cs, pin.huawei_power);
        MessageOutput.println(F("done"));
    } else {
        MessageOutput.println(F("Invalid pin config"));
    }

    Battery.init();
}

void loop()
{
    NetworkSettings.loop();
    yield();
    PowerMeter.loop();
    yield();
    PowerLimiter.loop();
    yield();
    InverterSettings.loop();
    yield();
    Datastore.loop();
    yield();
    // Vedirect_Enabled is unknown to lib. Therefor check has to be done here
    if (Configuration.get().Vedirect_Enabled) {
        for (int8_t i = 0; i < VICTRON_COUNT; i++)
        {
            VeDirectMppt[i].loop();
            yield();
        }    
    }
    MqttSettings.loop();
    yield();
    MqttHandleDtu.loop();
    yield();
    MqttHandleInverter.loop();
    yield();
    MqttHandleInverterTotal.loop();
    yield();
    MqttHandleVedirect.loop();
    yield();
    MqttHandleHass.loop();
    yield();
    MqttHandleVedirectHass.loop();
    yield();
    MqttHandleHuawei.loop();
    yield();
    MqttHandlePowerLimiter.loop();
    yield();
    WebApi.loop();
    yield();
    Display.loop();
    yield();
    SunPosition.loop();
    yield();
    MessageOutput.loop();
    yield();
    Battery.loop();
    yield();
    MqttHandlePylontechHass.loop();
    yield();
    HuaweiCan.loop();
    yield();
    LedSingle.loop();
    yield();
}
