#include <Arduino.h>
#include <GxEPD2_4G_4G.h>
#include <gdey/GxEPD2_213_GDEY0213B74.h>
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

GxEPD2_4G_4G<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT> display(GxEPD2_213_GDEY0213B74(/*CS=5*/ SS, /*DC=*/17, /*RST=*/16, /*BUSY=*/4));

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
    
    // Initialize display with 115200 baud rate
    display.init(115200);
    display.setRotation(1);
    
    // Clear the entire display first with white
    display.fillScreen(GxEPD_WHITE);
    
    // Set text properties
    display.setTextColor(GxEPD_BLACK);
    display.setTextSize(2);
    
    // Display battery status with dark gray background
    // display.fillRect(0, 0, display.width(), 30, GxEPD_LIGHTGREY);
    display.setTextColor(GxEPD_LIGHTGREY);
    display.setCursor(10, 20);
    display.print("Battery: ");
    display.println(getBatteryStatus());
    
    if (wifi.isConnected()) {
        // Display last update time with light gray background
        // display.fillRect(0, 30, display.width(), 30, GxEPD_DARKGREY);
        display.setTextColor(GxEPD_BLACK);
        display.setCursor(10, 50);
        display.print("Last update: ");
        display.println(lastUpdateTime);
        
        // Display weather with white background
        // display.fillRect(0, 60, display.width(), 40, GxEPD_WHITE);
        display.setCursor(10, 90);
        display.println(weatherData);
        
        // Display wind speed with light gray background
        // display.fillRect(0, 100, display.width(), 40, GxEPD_LIGHTGREY);
        display.setCursor(10, 120);
        display.println(windData);
    }
    
    // Update the entire display
    display.displayWindow(0, 0, display.width(), display.height());
    display.hibernate();
    Serial.println("Display updated");
}

void setup() {
    Serial.begin(115200);
    
    pinMode(BATTERY_PIN, INPUT);
    SPI.begin(18, 19, 23);
    
    // Initialize display with 115200 baud rate
    display.init(115200);
    display.setRotation(1);
    display.fillScreen(GxEPD_WHITE);
    display.displayWindow(0, 0, display.width(), display.height());
    display.hibernate();
    
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