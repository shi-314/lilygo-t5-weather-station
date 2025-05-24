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
                "&hourly=temperature_2m,precipitation,wind_speed_10m,wind_gusts_10m,cloud_cover_low" +
                "&current=wind_speed_10m,wind_gusts_10m,temperature_2m,weather_code,wind_direction_10m" +
                "&forecast_days=1" +
                "&timezone=auto";
    
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        Serial.println("Failed to get weather data");
        weatherData = "Weather error";
        hourlyTemperatures.clear();
        hourlyWindSpeeds.clear();
        hourlyWindGusts.clear();
        hourlyTime.clear();
        hourlyPrecipitation.clear();
        hourlyCloudCoverage.clear();
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
        hourlyWindGusts.clear();
        hourlyTime.clear();
        hourlyPrecipitation.clear();
        hourlyCloudCoverage.clear();
        http.end();
        return;
    }

    float temp = doc["current"]["temperature_2m"];
    int weatherCode = doc["current"]["weather_code"];
    String timeStr = doc["current"]["time"].as<String>();
    float windSpeedKmh = doc["current"]["wind_speed_10m"].as<float>();
    float windSpeed = windSpeedKmh / 3.6; // Convert km/h to m/s
    String windSpeedUnit = doc["current_units"]["wind_speed_10m"];
    windDirection = doc["current"]["wind_direction_10m"];
    float windGustsKmh = doc["current"]["wind_gusts_10m"].as<float>();
    float windGusts = windGustsKmh / 3.6; // Convert km/h to m/s
    // Serial.println(payload);
    
    currentTemperature = temp;
    currentWindSpeed = windSpeed;
    currentWindGusts = windGusts;
    currentWeatherDescription = getWeatherDescription(weatherCode);
    
    // Clear previous hourly data
    hourlyTemperatures.clear();
    hourlyWindSpeeds.clear();
    hourlyWindGusts.clear();
    hourlyTime.clear();
    hourlyPrecipitation.clear();
    hourlyCloudCoverage.clear();

    // Parse hourly temperature data
    JsonArray hourly_temp_array = doc["hourly"]["temperature_2m"].as<JsonArray>();
    for (JsonVariant v : hourly_temp_array) {
        hourlyTemperatures.push_back(v.as<float>());
    }

    // Parse hourly wind speed data
    JsonArray hourly_wind_array = doc["hourly"]["wind_speed_10m"].as<JsonArray>();
    for (JsonVariant v : hourly_wind_array) {
        hourlyWindSpeeds.push_back(v.as<float>() / 3.6);
    }

    // Parse hourly wind gusts data
    JsonArray hourly_wind_gusts_array = doc["hourly"]["wind_gusts_10m"].as<JsonArray>();
    for (JsonVariant v : hourly_wind_gusts_array) {
        hourlyWindGusts.push_back(v.as<float>() / 3.6);
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

    // Parse hourly cloud coverage data
    JsonArray hourly_cloud_array = doc["hourly"]["cloud_cover_low"].as<JsonArray>();
    for (JsonVariant v : hourly_cloud_array) {
        hourlyCloudCoverage.push_back(v.as<float>());
    }
    
    weatherData = String(temp, 1) + " C " + getWeatherDescription(weatherCode);
    
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

std::vector<float> Weather::getHourlyWindGusts() const {
    return hourlyWindGusts;
}

std::vector<String> Weather::getHourlyTime() const {
    return hourlyTime;
}

std::vector<float> Weather::getHourlyPrecipitation() const {
    return hourlyPrecipitation;
}

std::vector<float> Weather::getHourlyCloudCoverage() const {
    return hourlyCloudCoverage;
}

float Weather::getCurrentTemperature() const {
    return currentTemperature;
}

float Weather::getCurrentWindSpeed() const {
    return currentWindSpeed;
}

float Weather::getCurrentWindGusts() const {
    return currentWindGusts;
}

String Weather::getWeatherDescription() const {
    return currentWeatherDescription;
} 