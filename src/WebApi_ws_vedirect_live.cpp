// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_ws_vedirect_live.h"
#include "AsyncJson.h"
#include "Configuration.h"
#include "MessageOutput.h"
#include "WebApi.h"
#include "defaults.h"
#include "PowerLimiter.h"
#include <string.h>

WebApiWsVedirectLiveClass::WebApiWsVedirectLiveClass()
    : _ws("/vedirectlivedata")
{
}

void WebApiWsVedirectLiveClass::init(AsyncWebServer* server)
{
    for (int8_t i = 0; i < VICTRON_COUNT; i++)
    {
        _newestVedirectTimestamp[i] = 0;
    }
    

    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    using std::placeholders::_5;
    using std::placeholders::_6;
    
    _server = server;
    _server->on("/api/vedirectlivedata/status", HTTP_GET, std::bind(&WebApiWsVedirectLiveClass::onLivedataStatus, this, _1));

    _server->addHandler(&_ws);
    _ws.onEvent(std::bind(&WebApiWsVedirectLiveClass::onWebsocketEvent, this, _1, _2, _3, _4, _5, _6));
}

void WebApiWsVedirectLiveClass::loop()
{
    // see: https://github.com/me-no-dev/ESPAsyncWebServer#limiting-the-number-of-web-socket-clients
    if (millis() - _lastWsCleanup > 1000) {
        _ws.cleanupClients();
        _lastWsCleanup = millis();
    }

    // do nothing if no WS client is connected
    if (_ws.count() == 0) {
        return;
    }

    if (millis() - _lastVedirectUpdateCheck < 1000) {
        return;
    }
    _lastVedirectUpdateCheck = millis();

    bool immediateUpdate = false;
    for (int8_t i = 0; i < VICTRON_COUNT; i++)
    {
        if (VeDirectMppt[i].getLastUpdate() > 0 && VeDirectMppt[i].getLastUpdate() != _newestVedirectTimestamp[i])
        {
            immediateUpdate = true;
        }
    }

    // Update on ve.direct change or at least after 10 seconds
    if (millis() - _lastWsPublish > (10 * 1000) || immediateUpdate) {
        
        try {
            String buffer;
            // free JsonDocument as soon as possible
            {
                DynamicJsonDocument root(_responseSize * VICTRON_COUNT);
                JsonVariant var = root;
                generateJsonResponse(var);
                serializeJson(root, buffer);
            }
            
            if (buffer) {        
                if (Configuration.get().Security_AllowReadonly) {
                    _ws.setAuthentication("", "");
                } else {
                    _ws.setAuthentication(AUTH_USERNAME, Configuration.get().Security_Password);
                }

                _ws.textAll(buffer);
            }

        } catch (std::bad_alloc& bad_alloc) {
            MessageOutput.printf("Calling /api/vedirectlivedata/status has temporarily run out of resources. Reason: \"%s\".\r\n", bad_alloc.what());
        }

        _lastWsPublish = millis();
    }
}

void WebApiWsVedirectLiveClass::generateJsonResponse(JsonVariant& root)
{
    JsonArray mpptArray = root.createNestedArray("mppts");

// Loop all mppts
    for (uint8_t i = 0; i < VICTRON_COUNT; i++) {      

        if (!VeDirectMppt[i].isInit())
            continue;

        JsonObject mpptObject = mpptArray.createNestedObject();
    
        mpptObject["order"] = i;

        // device info
        mpptObject["device"]["data_age"] = (millis() - VeDirectMppt[i].getLastUpdate() ) / 1000;
        mpptObject["device"]["age_critical"] = !VeDirectMppt[i].isDataValid();
        mpptObject["device"]["PID"] = VeDirectMppt[i].getPidAsString(VeDirectMppt[i].veFrame.PID);
        mpptObject["device"]["SER"] = VeDirectMppt[i].veFrame.SER;
        mpptObject["device"]["FW"] = VeDirectMppt[i].veFrame.FW;
        mpptObject["device"]["LOAD"] = VeDirectMppt[i].veFrame.LOAD == true ? "ON" : "OFF";
        mpptObject["device"]["CS"] = VeDirectMppt[i].getCsAsString(VeDirectMppt[i].veFrame.CS);
        mpptObject["device"]["ERR"] = VeDirectMppt[i].getErrAsString(VeDirectMppt[i].veFrame.ERR);
        mpptObject["device"]["OR"] = VeDirectMppt[i].getOrAsString(VeDirectMppt[i].veFrame.OR);
        mpptObject["device"]["MPPT"] = VeDirectMppt[i].getMpptAsString(VeDirectMppt[i].veFrame.MPPT);
        mpptObject["device"]["HSDS"]["v"] = VeDirectMppt[i].veFrame.HSDS;
        mpptObject["device"]["HSDS"]["u"] = "d";

        // battery info    
        mpptObject["output"]["P"]["v"] = VeDirectMppt[i].veFrame.P;
        mpptObject["output"]["P"]["u"] = "W";
        mpptObject["output"]["P"]["d"] = 0;
        mpptObject["output"]["V"]["v"] = VeDirectMppt[i].veFrame.V;
        mpptObject["output"]["V"]["u"] = "V";
        mpptObject["output"]["V"]["d"] = 2;
        mpptObject["output"]["I"]["v"] = VeDirectMppt[i].veFrame.I;
        mpptObject["output"]["I"]["u"] = "A";
        mpptObject["output"]["I"]["d"] = 2;
        mpptObject["output"]["E"]["v"] = VeDirectMppt[i].veFrame.E;
        mpptObject["output"]["E"]["u"] = "%";
        mpptObject["output"]["E"]["d"] = 1;

        // panel info
        mpptObject["input"]["PPV"]["v"] = VeDirectMppt[i].veFrame.PPV;
        mpptObject["input"]["PPV"]["u"] = "W";
        mpptObject["input"]["PPV"]["d"] = 0;
        mpptObject["input"]["VPV"]["v"] = VeDirectMppt[i].veFrame.VPV;
        mpptObject["input"]["VPV"]["u"] = "V";
        mpptObject["input"]["VPV"]["d"] = 2;
        mpptObject["input"]["IPV"]["v"] = VeDirectMppt[i].veFrame.IPV;
        mpptObject["input"]["IPV"]["u"] = "A";
        mpptObject["input"]["IPV"]["d"] = 2;
        mpptObject["input"]["YieldToday"]["v"] = VeDirectMppt[i].veFrame.H20;
        mpptObject["input"]["YieldToday"]["u"] = "kWh";
        mpptObject["input"]["YieldToday"]["d"] = 3;
        mpptObject["input"]["YieldYesterday"]["v"] = VeDirectMppt[i].veFrame.H22;
        mpptObject["input"]["YieldYesterday"]["u"] = "kWh";
        mpptObject["input"]["YieldYesterday"]["d"] = 3;
        mpptObject["input"]["YieldTotal"]["v"] = VeDirectMppt[i].veFrame.H19;
        mpptObject["input"]["YieldTotal"]["u"] = "kWh";
        mpptObject["input"]["YieldTotal"]["d"] = 3;
        mpptObject["input"]["MaximumPowerToday"]["v"] = VeDirectMppt[i].veFrame.H21;
        mpptObject["input"]["MaximumPowerToday"]["u"] = "W";
        mpptObject["input"]["MaximumPowerToday"]["d"] = 0;
        mpptObject["input"]["MaximumPowerYesterday"]["v"] = VeDirectMppt[i].veFrame.H23;
        mpptObject["input"]["MaximumPowerYesterday"]["u"] = "W";
        mpptObject["input"]["MaximumPowerYesterday"]["d"] = 0; 
        
        if (VeDirectMppt[i].getLastUpdate() > _newestVedirectTimestamp[i]) {
            _newestVedirectTimestamp[i] = VeDirectMppt[i].getLastUpdate();
        }
    }

    // power limiter state
        root["dpl"]["PLSTATE"] = -1;
        if (Configuration.get().PowerLimiter_Enabled)
            root["dpl"]["PLSTATE"] = PowerLimiter.getPowerLimiterState();
        root["dpl"]["PLLIMIT"] = PowerLimiter.getLastRequestedPowerLimit();

}

void WebApiWsVedirectLiveClass::onWebsocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len)
{
    if (type == WS_EVT_CONNECT) {
        char str[64];
        snprintf(str, sizeof(str), "Websocket: [%s][%u] connect", server->url(), client->id());
        Serial.println(str);
        MessageOutput.println(str);
    } else if (type == WS_EVT_DISCONNECT) {
        char str[64];
        snprintf(str, sizeof(str), "Websocket: [%s][%u] disconnect", server->url(), client->id());
        Serial.println(str);
        MessageOutput.println(str);
    }
}

void WebApiWsVedirectLiveClass::onLivedataStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }
    try {
        AsyncJsonResponse* response = new AsyncJsonResponse(false, _responseSize * VICTRON_COUNT);
        JsonVariant root = response->getRoot();

        generateJsonResponse(root);

        response->setLength();
        request->send(response);

    } catch (std::bad_alloc& bad_alloc) {
        MessageOutput.printf("Calling /api/livedata/status has temporarily run out of resources. Reason: \"%s\".\r\n", bad_alloc.what());

        WebApi.sendTooManyRequests(request);
    }
}