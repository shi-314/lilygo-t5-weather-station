#include <Arduino.h>
#include <GxEPD2_4G_4G.h>
#include <gdey/GxEPD2_213_GDEY0213B74.h>
#include <time.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "battery.h"
#include "wifi_connection.h"
#include <Fonts/FreeSans9pt7b.h>  // Smaller font for general text
#include <Fonts/FreeSans12pt7b.h> // Larger font for weather data
#include "boards.h"  // Include for LED pin definition
#include "assets/WifiErrorIcon.h"

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
const unsigned long sleepTime = 300000000; // Deep sleep time in microseconds (5 minutes)

WiFiConnection wifi(ssid, password);

// Forward declarations
void updateWeather();
void updateDisplay();
void goToSleep();
void displayWifiErrorIcon();

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
    
    display.init(115200);
    display.setRotation(1);
    display.fillScreen(GxEPD_WHITE);

    display.setFont(&FreeSans9pt7b);
    display.setTextColor(GxEPD_LIGHTGREY);

    display.setCursor(10, 20);
    display.print("Battery: ");
    display.println(getBatteryStatus());
    
    // Only called when WiFi is connected
    display.setFont(&FreeSans9pt7b);
    display.setTextColor(GxEPD_LIGHTGREY);

    display.setCursor(10, 45);
    display.print("Last update: ");
    display.println(lastUpdateTime);
    
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeSans12pt7b);

    display.setCursor(10, 100); 
    display.println(weatherData);
    
    display.setCursor(10, 125);
    display.println(windData);
    
    // Update the entire display
    display.displayWindow(0, 0, display.width(), display.height());
    display.hibernate();
    Serial.println("Display updated");
}

void goToSleep() {
    Serial.println("Going to deep sleep for " + String(sleepTime / 1000000) + " seconds");
    
    esp_sleep_enable_timer_wakeup(sleepTime);
    esp_deep_sleep_start();
}

void displayWifiErrorIcon() {
    Serial.println("Displaying WiFi error icon");
    
    display.init(115200);
    display.setRotation(1);  // Ensure consistent rotation
    display.fillScreen(GxEPD_WHITE);
    
    // WifiErrorIcon is 16x16 pixels
    int iconWidth = 16;
    int iconHeight = 16;
    
    // Calculate center position to place the icon exactly in the center
    int centerX = (display.width() / 2) - (iconWidth / 2);
    int centerY = (display.height() / 2) - (iconHeight / 2);
    
    // Draw the icon at the center
    display.drawBitmap(centerX, centerY, WifiErrorIcon, iconWidth, iconHeight, GxEPD_BLACK);
    
    // Full update to ensure the icon is properly displayed
    display.displayWindow(0, 0, display.width(), display.height());
    display.hibernate();
}

void setup() {
    Serial.begin(115200);
    
    pinMode(BATTERY_PIN, INPUT);
    SPI.begin(18, 19, 23);
    
    display.init(115200);
    display.setRotation(1);
    display.fillScreen(GxEPD_WHITE);
    display.hibernate();
    
    wifi.connect();
    if (!wifi.isConnected()) {
        Serial.println("Failed to connect to WiFi");
        displayWifiErrorIcon();
        goToSleep();
        return;
    }
    
    updateWeather();
    updateDisplay();
    
    goToSleep();
}

void loop() {
    // Most of the work is done in setup() followed by deep sleep
    // The loop() function won't be executed unless something prevents deep sleep
} 