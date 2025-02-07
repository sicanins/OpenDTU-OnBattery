// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Helge Erbe and others
 */
#include "VeDirectMpptController.h"
#include "MqttHandleVedirect.h"
#include "MqttSettings.h"
#include "MessageOutput.h"




MqttHandleVedirectClass MqttHandleVedirect;

// #define MQTTHANDLEVEDIRECT_DEBUG

void MqttHandleVedirectClass::init()
{
    // initially force a full publish
    _nextPublishUpdatesOnly = 0;
    _nextPublishFull = 1;
}

void MqttHandleVedirectClass::loop()
{
    CONFIG_T& config = Configuration.get();

    if (!MqttSettings.getConnected() || !config.Vedirect_Enabled) {
        return;
    }   

    if ((millis() >= _nextPublishFull) || (millis() >= _nextPublishUpdatesOnly)) {
        // determine if this cycle should publish full values or updates only
        if (_nextPublishFull <= _nextPublishUpdatesOnly) {
            _PublishFull = true;
        } else {
            _PublishFull = !config.Vedirect_UpdatesOnly;
        }

        #ifdef MQTTHANDLEVEDIRECT_DEBUG
        MessageOutput.printf("\r\n\r\nMqttHandleVedirectClass::loop millis %lu   _nextPublishUpdatesOnly %u   _nextPublishFull %u\r\n", millis(), _nextPublishUpdatesOnly, _nextPublishFull);
        if (_PublishFull) {
            MessageOutput.println("MqttHandleVedirectClass::loop publish full");
        } else {
            MessageOutput.println("MqttHandleVedirectClass::loop publish updates only");
        }
        #endif

        String topic_root = "victron/";  

        int8_t count = 0;
        double E_total = 0;
        int32_t PPV_total = 0; 
        int32_t P_total = 0; 
        double H19_total = 0;
        double H20_total = 0;
        double H21_total = 0;
        double H22_total = 0;

        for (int8_t i = 0; i < VICTRON_COUNT; i++)
        {
            //if (!VeDirectMppt[i].isDataValid()) 
            //    continue;
    
            count++;

            String value;           
            String topic = "";
            topic.concat(topic_root);
            topic.concat(VeDirectMppt[i].veFrame.SER);
            topic.concat("/");

            E_total += VeDirectMppt[i].veFrame.E;
            PPV_total += VeDirectMppt[i].veFrame.PPV;
            P_total += VeDirectMppt[i].veFrame.P;
            H19_total += VeDirectMppt[i].veFrame.H19;
            H20_total += VeDirectMppt[i].veFrame.H20;
            H21_total += VeDirectMppt[i].veFrame.H21;
            H22_total += VeDirectMppt[i].veFrame.H22;

            if (_PublishFull || VeDirectMppt[i].veFrame.PID != _kvFrame[i].PID)
                MqttSettings.publish(topic + "PID", VeDirectMppt[i].getPidAsString(VeDirectMppt[i].veFrame.PID)); 
            if (_PublishFull || strcmp(VeDirectMppt[i].veFrame.SER, _kvFrame[i].SER) != 0)
                MqttSettings.publish(topic + "SER", VeDirectMppt[i].veFrame.SER ); 
            if (_PublishFull || strcmp(VeDirectMppt[i].veFrame.FW, _kvFrame[i].FW) != 0)
                MqttSettings.publish(topic + "FW", VeDirectMppt[i].veFrame.FW); 
            if (_PublishFull || VeDirectMppt[i].veFrame.LOAD != _kvFrame[i].LOAD)
                MqttSettings.publish(topic + "LOAD", VeDirectMppt[i].veFrame.LOAD == true ? "ON": "OFF"); 
            if (_PublishFull || VeDirectMppt[i].veFrame.CS != _kvFrame[i].CS)
                MqttSettings.publish(topic + "CS", VeDirectMppt[i].getCsAsString(VeDirectMppt[i].veFrame.CS)); 
            if (_PublishFull || VeDirectMppt[i].veFrame.ERR != _kvFrame[i].ERR)
                MqttSettings.publish(topic + "ERR", VeDirectMppt[i].getErrAsString(VeDirectMppt[i].veFrame.ERR)); 
            if (_PublishFull || VeDirectMppt[i].veFrame.OR != _kvFrame[i].OR)
                MqttSettings.publish(topic + "OR", VeDirectMppt[i].getOrAsString(VeDirectMppt[i].veFrame.OR)); 
            if (_PublishFull || VeDirectMppt[i].veFrame.MPPT != _kvFrame[i].MPPT)
                MqttSettings.publish(topic + "MPPT", VeDirectMppt[i].getMpptAsString(VeDirectMppt[i].veFrame.MPPT)); 
            if (_PublishFull || VeDirectMppt[i].veFrame.HSDS != _kvFrame[i].HSDS) {
                value = VeDirectMppt[i].veFrame.HSDS;
                MqttSettings.publish(topic + "HSDS", value); 
            }
            if (_PublishFull || VeDirectMppt[i].veFrame.V != _kvFrame[i].V) {
                value = VeDirectMppt[i].veFrame.V;
                MqttSettings.publish(topic + "V", value); 
            }
            if (_PublishFull || VeDirectMppt[i].veFrame.I != _kvFrame[i].I) {
                value = VeDirectMppt[i].veFrame.I;
                MqttSettings.publish(topic + "I", value); 
            }
            if (_PublishFull || VeDirectMppt[i].veFrame.P != _kvFrame[i].P) {
                value = VeDirectMppt[i].veFrame.P;
                MqttSettings.publish(topic + "P", value); 
            }
            if (_PublishFull || VeDirectMppt[i].veFrame.VPV != _kvFrame[i].VPV) {
                value = VeDirectMppt[i].veFrame.VPV;
                MqttSettings.publish(topic + "VPV", value); 
            }
            if (_PublishFull || VeDirectMppt[i].veFrame.PPV != _kvFrame[i].PPV) {
                value = VeDirectMppt[i].veFrame.PPV;
                MqttSettings.publish(topic + "PPV", value); 
            }
            if (_PublishFull || VeDirectMppt[i].veFrame.H19 != _kvFrame[i].H19) {
                value = VeDirectMppt[i].veFrame.H19;
                MqttSettings.publish(topic + "H19", value); 
            }
            if (_PublishFull || VeDirectMppt[i].veFrame.H20 != _kvFrame[i].H20) {
                value = VeDirectMppt[i].veFrame.H20;
                MqttSettings.publish(topic + "H20", value); 
            }
            if (_PublishFull || VeDirectMppt[i].veFrame.H21 != _kvFrame[i].H21) {
                value = VeDirectMppt[i].veFrame.H21;
                MqttSettings.publish(topic + "H21", value); 
            }
            if (_PublishFull || VeDirectMppt[i].veFrame.H22 != _kvFrame[i].H22) {
                value = VeDirectMppt[i].veFrame.H22;
                MqttSettings.publish(topic + "H22", value); 
            }
            if (_PublishFull || VeDirectMppt[i].veFrame.H23 != _kvFrame[i].H23) {
                value = VeDirectMppt[i].veFrame.H23;
                MqttSettings.publish(topic + "H23", value); 
            }
            if (_PublishFull || VeDirectMppt[i].veFrame.E != _kvFrame[i].E) {
                value = VeDirectMppt[i].veFrame.E;
                MqttSettings.publish(topic + "E", value); 
            }
            if (!_PublishFull) {
                _kvFrame[i] = VeDirectMppt[i].veFrame;
            }
        }
        
        String value;
        value = PPV_total;
        MqttSettings.publish(topic_root + "PPV_total",  value); 
        value = P_total;
        MqttSettings.publish(topic_root + "P_total",  value); 

        value = E_total/count;
        MqttSettings.publish(topic_root + "E_total", value);  

        value = H19_total;
        MqttSettings.publish(topic_root + "H19_total",  value); 
        value = H20_total;
        MqttSettings.publish(topic_root + "H20_total",  value); 
        value = H21_total;
        MqttSettings.publish(topic_root + "H21_total",  value); 
        value = H22_total;
        MqttSettings.publish(topic_root + "H22_total",  value); 

        // now calculate next points of time to publish
        _nextPublishUpdatesOnly = millis() + (config.Mqtt_PublishInterval * 1000);

        if (_PublishFull) {
            // when Home Assistant MQTT-Auto-Discovery is active,
            // and "enable expiration" is active, all values must be published at
            // least once before the announced expiry interval is reached
            if ((config.Vedirect_UpdatesOnly) && (config.Mqtt_Hass_Enabled) && (config.Mqtt_Hass_Expire)) {
                _nextPublishFull = millis() + (((config.Mqtt_PublishInterval * 3) - 1) * 1000);

                #ifdef MQTTHANDLEVEDIRECT_DEBUG
                uint32_t _tmpNextFullSeconds = (config.Mqtt_PublishInterval * 3) - 1;
                MessageOutput.printf("MqttHandleVedirectClass::loop _tmpNextFullSeconds %u - _nextPublishFull %u \r\n", _tmpNextFullSeconds, _nextPublishFull);
                #endif

            } else {
                // no future publish full needed
                _nextPublishFull = UINT32_MAX;
            }
        }

        #ifdef MQTTHANDLEVEDIRECT_DEBUG
        MessageOutput.printf("MqttHandleVedirectClass::loop _nextPublishUpdatesOnly %u   _nextPublishFull %u\r\n", _nextPublishUpdatesOnly, _nextPublishFull);
        #endif
    }
}