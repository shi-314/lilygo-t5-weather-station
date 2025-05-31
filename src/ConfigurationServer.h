#ifndef CONFIGURATION_SERVER_H
#define CONFIGURATION_SERVER_H

#include <Arduino.h>

class ConfigurationServer {
public:
    ConfigurationServer();
    void run();

private:
    String deviceName;
    String wifiNetworkName;
    String wifiPassword;
};

#endif 