// Copyright (C) 2026 C-E
#pragma once

#include <ESPAsyncWebServer.h>

class WebApiLimitClass {
public:
    void init(AsyncWebServer& server);
    void loop();

private:
    void onLimitStatus(AsyncWebServerRequest* request);
    void onLimitPost(AsyncWebServerRequest* request);
    void SetAutoMaxLeistung(AsyncWebServerRequest* request); // ce-change: added

    AsyncWebServer* _server;
};