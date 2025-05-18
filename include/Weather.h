#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Forward declaration (actual WiFiConnection is expected to be defined elsewhere)
class WiFiConnection;

class Weather {
public:
    Weather(float latitude, float longitude);
    
    // Uses the global wifi object to check connectivity before updating
    void update();
    String getWeatherText() const;
    String getWindText() const;
    String getLastUpdateTime() const;
    bool isTimeToUpdate(unsigned long currentMillis) const;

private:
    // API settings
    const char* openMeteoEndpoint = "https://api.open-meteo.com/v1/forecast";
    const float latitude;
    const float longitude;
    
    // Weather data
    String weatherData = "Loading...";
    String windData = "Loading...";
    String lastUpdateTime = "";
    unsigned long lastWeatherUpdate = 0;
    const unsigned long updateInterval = 300000; // Update weather every 5 minutes
    
    String getWeatherDescription(int weatherCode) const;
}; 