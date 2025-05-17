#ifndef WIFI_CONNECTION_H
#define WIFI_CONNECTION_H

#include <Arduino.h>
#include <WiFi.h>
#include <HardwareSerial.h>

class WiFiConnection {
private:
    const char* _ssid;
    const char* _password;
    bool connected;

public:
    WiFiConnection(const char* ssid, const char* password);
    void connect();
    void reconnect();
    bool isConnected();
    void checkConnection();
};

#endif // WIFI_CONNECTION_H 