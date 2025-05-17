#include <Arduino.h>
#include <GxEPD.h>
#include <GxDEPG0213BN/GxDEPG0213BN.h>    // 2.13" b/w  form DKE GROUP
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>
#include <time.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "battery.h"
#include "wifi_connection.h"

// Define pins for the display
#define EPD_CS 5
#define EPD_DC 17
#define EPD_RSET 16
#define EPD_BUSY 4

// WiFi credentials
const char* ssid = ":(";
const char* password = "20009742591595504581";

// Open-Meteo API settings
const char* openMeteoEndpoint = "https://api.open-meteo.com/v1/forecast";
const float latitude = 52.520008;  // Berlin latitude
const float longitude = 13.404954; // Berlin longitude

GxIO_Class io(SPI, EPD_CS, EPD_DC, EPD_RSET);
GxEPD_Class display(io, EPD_RSET, EPD_BUSY);

String weatherData = "Loading...";
String windData = "Loading...";
String lastUpdateTime = "";
unsigned long lastWeatherUpdate = 0;
const unsigned long weatherUpdateInterval = 300000; // Update weather every 5 minutes
const unsigned long displayUpdateInterval = 60000;  // Update display every minute

WiFiConnection wifi(ssid, password);

String getWeatherDescription(int weatherCode) {
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

void updateWeather() {
    if (!wifi.isConnected()) {
        Serial.println("WiFi not connected");
        return;
    }

    HTTPClient http;
    String url = String(openMeteoEndpoint) + 
                "?latitude=" + String(latitude, 6) + 
                "&longitude=" + String(longitude, 6) + 
                "&current_weather=true" +
                "&current=wind_speed_10m" +
                "&timezone=auto";
    
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        Serial.println("Failed to get weather data");
        weatherData = "Weather error";
        http.end();
        return;
    }

    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
        weatherData = "JSON error";
        http.end();
        return;
    }

    float temp = doc["current_weather"]["temperature"];
    int weatherCode = doc["current_weather"]["weathercode"];
    String timeStr = doc["current_weather"]["time"].as<String>();
    float windSpeed = doc["current_weather"]["windspeed"];
    String windSpeedUnit = doc["current_weather_units"]["windspeed"];
    
    weatherData = String(temp, 1) + " C " + getWeatherDescription(weatherCode);
    windData = String(windSpeed, 1) + " " + windSpeedUnit;
    
    struct tm timeinfo;
    strptime(timeStr.c_str(), "%Y-%m-%dT%H:%M", &timeinfo);
    char timeBuffer[6];
    strftime(timeBuffer, sizeof(timeBuffer), "%H:%M", &timeinfo);
    lastUpdateTime = String(timeBuffer);
    
    http.end();
}

void updateDisplay() {
    Serial.println("Updating display...");
    
    // Clear the entire display first
    display.fillScreen(GxEPD_WHITE);
    
    // Set text properties
    display.setTextColor(GxEPD_BLACK);
    display.setTextSize(2);
    
    // Display battery status
    display.setCursor(10, 20);
    display.print("Battery: ");
    display.println(getBatteryStatus());
    
    if (wifi.isConnected()) {
        // Display last update time
        display.setCursor(10, 40);
        display.print("Last update: ");
        display.println(lastUpdateTime);
        
        // Display weather
        display.setCursor(10, 90);
        display.println(weatherData);
        
        // Display wind speed
        display.setCursor(10, 110);
        display.println(windData);
    }
    
    // Update the entire display
    display.update();
    Serial.println("Display updated");
}

void setup() {
    Serial.begin(115200);
    
    pinMode(BATTERY_PIN, INPUT);
    SPI.begin(18, 19, 23);
    
    display.init();
    display.setRotation(1);
    display.fillScreen(GxEPD_WHITE);
    display.update();
    
    wifi.connect();
    if (!wifi.isConnected()) {
        Serial.println("Failed to connect to WiFi");
        return;
    }
    
    updateWeather();
    lastWeatherUpdate = millis();
    updateDisplay();
}

void loop() {
    wifi.checkConnection();
    unsigned long currentMillis = millis();
    
    if (wifi.isConnected() && currentMillis - lastWeatherUpdate >= weatherUpdateInterval) {
        updateWeather();
        lastWeatherUpdate = currentMillis;
        updateDisplay();
    }
    
    delay(100);
} 