#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

#include <vector>

struct GeocodingResult {
  String name;
  float latitude;
  float longitude;
  float elevation;
  String countryCode;
};

struct WeatherForecastToday {
  String lastUpdateTime;

  float currentTemperature;
  float currentWindSpeed;
  float currentWindGusts;
  int currentWindDirection;
  String currentWeatherDescription;
  int currentWeatherCode;
  String currentWeatherCodeDescription;

  std::vector<float> hourlyTemperatures;
  std::vector<float> hourlyWindSpeeds;
  std::vector<float> hourlyWindGusts;
  std::vector<String> hourlyTime;
  std::vector<float> hourlyPrecipitation;
  std::vector<float> hourlyCloudCoverage;

  String apiPayload;
};

class OpenMeteoAPI {
 public:
  OpenMeteoAPI(float latitude, float longitude);

  void update();
  String getWeatherText() const;
  String getLastUpdateTime() const;
  int getWindDirection() const;
  std::vector<float> getHourlyTemperatures() const;
  std::vector<float> getHourlyWindSpeeds() const;
  std::vector<float> getHourlyWindGusts() const;
  std::vector<String> getHourlyTime() const;
  std::vector<float> getHourlyPrecipitation() const;
  std::vector<float> getHourlyCloudCoverage() const;

  float getCurrentTemperature() const;
  float getCurrentWindSpeed() const;
  float getCurrentWindGusts() const;
  String getWeatherDescription() const;
  String getLastPayload() const;

  GeocodingResult getLocationByCity(const String& cityName, const String& countryCode = "") const;
  WeatherForecastToday getForecast(float latitude, float longitude) const;

 private:
  // API settings
  const char* forecastEndpoint = "https://api.open-meteo.com/v1/forecast";
  const char* geocodingEndpoint = "https://geocoding-api.open-meteo.com/v1/search";
  const float latitude;
  const float longitude;

  // Weather data
  String weatherData = "Loading...";
  String lastUpdateTime = "";
  int windDirection = 0;

  float currentTemperature = 0.0;
  float currentWindSpeed = 0.0;
  float currentWindGusts = 0.0;
  String currentWeatherDescription = "Loading...";

  std::vector<float> hourlyTemperatures;
  std::vector<float> hourlyWindSpeeds;
  std::vector<float> hourlyWindGusts;
  std::vector<String> hourlyTime;
  std::vector<float> hourlyPrecipitation;
  std::vector<float> hourlyCloudCoverage;

  String lastApiPayload = "";

  String getWeatherDescription(int weatherCode) const;
};