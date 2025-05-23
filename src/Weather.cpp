#include "Weather.h"
#include <time.h>

Weather::Weather(float latitude, float longitude) 
    : latitude(latitude), longitude(longitude) {
}

void Weather::update() {
    HTTPClient http;
    String url = String(openMeteoEndpoint) + 
                "?latitude=" + String(latitude, 6) + 
                "&longitude=" + String(longitude, 6) + 
                "&current_weather=true" +
                "&current=wind_speed_10m" +
                "&hourly=temperature_2m,wind_speed_10m,precipitation" +
                "&timezone=auto";
    
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        Serial.println("Failed to get weather data");
        weatherData = "Weather error";
        hourlyTemperatures.clear();
        hourlyWindSpeeds.clear();
        hourlyTime.clear();
        hourlyPrecipitation.clear();
        http.end();
        return;
    }

    String payload = http.getString();
    DynamicJsonDocument doc(16384);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
        weatherData = "JSON error";
        hourlyTemperatures.clear();
        hourlyWindSpeeds.clear();
        hourlyTime.clear();
        hourlyPrecipitation.clear();
        http.end();
        return;
    }

    float temp = doc["current_weather"]["temperature"];
    int weatherCode = doc["current_weather"]["weathercode"];
    String timeStr = doc["current_weather"]["time"].as<String>();
    float windSpeed = doc["current_weather"]["windspeed"];
    String windSpeedUnit = doc["current_weather_units"]["windspeed"];
    windDirection = doc["current_weather"]["winddirection"];
    
    currentTemperature = temp;
    currentWindSpeed = windSpeed;
    currentWeatherDescription = getWeatherDescription(weatherCode);
    
    // Clear previous hourly data
    hourlyTemperatures.clear();
    hourlyWindSpeeds.clear();
    hourlyTime.clear();
    hourlyPrecipitation.clear();

    // Parse hourly temperature data
    JsonArray hourly_temp_array = doc["hourly"]["temperature_2m"].as<JsonArray>();
    for (JsonVariant v : hourly_temp_array) {
        hourlyTemperatures.push_back(v.as<float>());
    }

    // Parse hourly wind speed data
    JsonArray hourly_wind_array = doc["hourly"]["wind_speed_10m"].as<JsonArray>();
    for (JsonVariant v : hourly_wind_array) {
        hourlyWindSpeeds.push_back(v.as<float>());
    }

    // Parse hourly time data
    JsonArray hourly_time_array = doc["hourly"]["time"].as<JsonArray>();
    for (JsonVariant v : hourly_time_array) {
        String fullTimestamp = v.as<String>();
        // Extract just HH:MM for display
        hourlyTime.push_back(fullTimestamp.substring(11, 16)); 
    }

    // Parse hourly precipitation data
    JsonArray hourly_precipitation_array = doc["hourly"]["precipitation"].as<JsonArray>();
    for (JsonVariant v : hourly_precipitation_array) {
        hourlyPrecipitation.push_back(v.as<float>());
    }
    
    weatherData = String(temp, 1) + " C " + getWeatherDescription(weatherCode);
    windData = String(windSpeed, 1) + " " + windSpeedUnit;
    
    struct tm timeinfo;
    strptime(timeStr.c_str(), "%Y-%m-%dT%H:%M", &timeinfo);
    char timeBuffer[6];
    strftime(timeBuffer, sizeof(timeBuffer), "%H:%M", &timeinfo);
    lastUpdateTime = String(timeBuffer);
    
    lastWeatherUpdate = millis();
    
    http.end();
}

String Weather::getWeatherDescription(int weatherCode) const {
    switch (weatherCode) {
        case 0: return "Clear";
        case 1: case 2: case 3: return "Cloudy";
        case 45: case 48: return "Foggy";
        case 51: case 53: case 55: return "Drizzle";
        case 61: case 63: case 65: return "Rain";
        case 71: case 73: case 75: return "Snow";
        case 77: return "Snow grains";
        case 80: case 81: case 82: return "Rain showers";
        case 85: case 86: return "Snow showers";
        case 95: return "Thunderstorm";
        case 96: case 99: return "Thunderstorm with hail";
        default: return "Unknown";
    }
}

String Weather::getWeatherText() const {
    return weatherData;
}

String Weather::getWindText() const {
    return windData;
}

String Weather::getLastUpdateTime() const {
    return lastUpdateTime;
}

bool Weather::isTimeToUpdate(unsigned long currentMillis) const {
    return (currentMillis - lastWeatherUpdate) >= updateInterval || lastWeatherUpdate == 0;
}

int Weather::getWindDirection() const {
    return windDirection;
}

std::vector<float> Weather::getHourlyTemperatures() const {
    return hourlyTemperatures;
}

std::vector<float> Weather::getHourlyWindSpeeds() const {
    return hourlyWindSpeeds;
}

std::vector<String> Weather::getHourlyTime() const {
    return hourlyTime;
}

std::vector<float> Weather::getHourlyPrecipitation() const {
    return hourlyPrecipitation;
}

float Weather::getCurrentTemperature() const {
    return currentTemperature;
}

float Weather::getCurrentWindSpeed() const {
    return currentWindSpeed;
}

String Weather::getWeatherDescription() const {
    return currentWeatherDescription;
} 