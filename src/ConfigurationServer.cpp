#include "ConfigurationServer.h"
#include <WiFi.h>
#include <WiFiAP.h>

ConfigurationServer::ConfigurationServer() 
    : deviceName("LilyGo-Weather-Station"),
      wifiNetworkName("WeatherStation-Config"),
      wifiPassword("configure123") {
}

void ConfigurationServer::run() {
    Serial.println("Starting Configuration Server...");
    Serial.print("Device Name: ");
    Serial.println(deviceName);
    
    // Stop any existing WiFi connection
    WiFi.disconnect(true);
    delay(1000);
    
    Serial.print("Setting up WiFi Access Point: ");
    Serial.println(wifiNetworkName);
    
    bool apStarted = WiFi.softAP(wifiNetworkName.c_str(), wifiPassword.c_str());
    
    if (apStarted) {
        Serial.println("Access Point started successfully!");
        Serial.print("Network Name (SSID): ");
        Serial.println(wifiNetworkName);
        Serial.print("Password: ");
        Serial.println(wifiPassword);
        Serial.print("Access Point IP: ");
        Serial.println(WiFi.softAPIP());
        Serial.println("Other devices can now connect to this network");
    } else {
        Serial.println("Failed to start Access Point!");
    }
} 