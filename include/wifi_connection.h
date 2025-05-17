#pragma once

#include <Arduino.h>
#include <WiFi.h>

class WiFiConnection {
public:
    WiFiConnection(const char* ssid, const char* password);
    void connect();
    void reconnect();
    bool isConnected();
    void checkConnection();

private:
    const char* _ssid;
    const char* _password;
    bool connected;
}; 