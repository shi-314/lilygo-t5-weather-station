#include <Arduino.h>
#include <GxEPD.h>
#include <GxDEPG0213BN/GxDEPG0213BN.h>    // 2.13" b/w  form DKE GROUP
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>
#include <WiFi.h>
#include <time.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Define pins for the display
#define EPD_CS 5
#define EPD_DC 17
#define EPD_RSET 16
#define EPD_BUSY 4
#define BATTERY_PIN 35  // ADC pin for battery voltage

// Battery voltage constants
#define BATTERY_MAX_VOLTAGE 4.2  // Maximum battery voltage
#define BATTERY_MIN_VOLTAGE 3.3  // Minimum battery voltage
#define VOLTAGE_DIVIDER_RATIO 2.0  // Voltage divider ratio (if used)

// WiFi credentials
const char* ssid = ":(";
const char* password = "20009742591595504581";

// Open-Meteo API settings
const char* openMeteoEndpoint = "https://api.open-meteo.com/v1/forecast";
const float latitude = 52.520008;  // Berlin latitude
const float longitude = 13.404954; // Berlin longitude

GxIO_Class io(SPI, EPD_CS, EPD_DC, EPD_RSET);
GxEPD_Class display(io, EPD_RSET, EPD_BUSY);

bool wifiConnected = false;
String weatherData = "Loading...";
String windData = "Loading...";
String lastUpdateTime = "";
unsigned long lastWeatherUpdate = 0;
const unsigned long weatherUpdateInterval = 300000; // Update weather every 5 minutes
const unsigned long displayUpdateInterval = 60000;  // Update display every minute

String getBatteryStatus() {
    // Read battery voltage
    int rawValue = analogRead(BATTERY_PIN);
    float voltage = (rawValue * 3.3) / 4095.0;  // Convert to voltage (ESP32 ADC is 12-bit, 0-3.3V)
    voltage *= VOLTAGE_DIVIDER_RATIO;  // Adjust for voltage divider if used
    
    // Calculate percentage
    int percentage = map(voltage * 100, BATTERY_MIN_VOLTAGE * 100, BATTERY_MAX_VOLTAGE * 100, 0, 100);
    percentage = constrain(percentage, 0, 100);  // Ensure percentage is between 0 and 100
    
    return String(percentage) + "%";
}

void updateWeather() {
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Updating weather data...");
        HTTPClient http;
        String url = String(openMeteoEndpoint) + 
                    "?latitude=" + String(latitude, 6) + 
                    "&longitude=" + String(longitude, 6) + 
                    "&current_weather=true" +
                    "&current=wind_speed_10m" +
                    "&timezone=auto";
        
        Serial.print("Requesting URL: ");
        Serial.println(url);
        
        http.begin(url);
        int httpCode = http.GET();
        
        Serial.print("HTTP Response code: ");
        Serial.println(httpCode);
        
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            Serial.println("Received payload:");
            Serial.println(payload);
            
            DynamicJsonDocument doc(1024);
            deserializeJson(doc, payload);
            
            // Get current temperature and weather code
            float temp = doc["current_weather"]["temperature"];
            int weatherCode = doc["current_weather"]["weathercode"];
            String timeStr = doc["current_weather"]["time"].as<String>();
            float windSpeed = doc["current_weather"]["windspeed"];
            String windSpeedUnit = doc["current_weather_units"]["windspeed"];
            
            Serial.print("Temperature: ");
            Serial.println(temp);
            Serial.print("Weather code: ");
            Serial.println(weatherCode);
            Serial.print("Wind speed: ");
            Serial.println(windSpeed);
            Serial.print("Time: ");
            Serial.println(timeStr);
            
            // Convert weather code to description
            String weatherDesc;
            switch (weatherCode) {
                case 0: weatherDesc = "Clear"; break;
                case 1: case 2: case 3: weatherDesc = "Cloudy"; break;
                case 45: case 48: weatherDesc = "Foggy"; break;
                case 51: case 53: case 55: weatherDesc = "Drizzle"; break;
                case 61: case 63: case 65: weatherDesc = "Rain"; break;
                case 71: case 73: case 75: weatherDesc = "Snow"; break;
                case 77: weatherDesc = "Snow grains"; break;
                case 80: case 81: case 82: weatherDesc = "Rain showers"; break;
                case 85: case 86: weatherDesc = "Snow showers"; break;
                case 95: weatherDesc = "Thunderstorm"; break;
                case 96: case 99: weatherDesc = "Thunderstorm with hail"; break;
                default: weatherDesc = "Unknown"; break;
            }
            
            weatherData = String(temp, 1) + " C " + weatherDesc;
            windData = String(windSpeed, 1) + " " + windSpeedUnit;
            
            // Parse ISO 8601 time format using strptime
            struct tm timeinfo;
            strptime(timeStr.c_str(), "%Y-%m-%dT%H:%M", &timeinfo);
            char timeBuffer[6];
            strftime(timeBuffer, sizeof(timeBuffer), "%H:%M", &timeinfo);
            lastUpdateTime = String(timeBuffer);
            
            Serial.print("Weather data: ");
            Serial.println(weatherData);
            Serial.print("Last update time: ");
            Serial.println(lastUpdateTime);
        } else {
            Serial.println("Failed to get weather data");
            weatherData = "Weather error";
        }
        
        http.end();
    } else {
        Serial.println("WiFi not connected, cannot update weather");
    }
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
    
    if (wifiConnected) {
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

void connectToWiFi() {
    Serial.print("Connecting to WiFi");
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to WiFi");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        wifiConnected = true;
        
        // Get initial weather data
        updateWeather();
    } else {
        Serial.println("\nFailed to connect to WiFi");
        wifiConnected = false;
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starting...");

    // Initialize battery ADC
    pinMode(BATTERY_PIN, INPUT);
    
    // Initialize SPI
    SPI.begin(18, 19, 23);  // SCK, MISO, MOSI

    // Initialize display
    display.init();
    Serial.println("Display initialized");

    // Set rotation to landscape (1 = 90 degrees clockwise)
    display.setRotation(1);
    
    // Initial full display update
    display.fillScreen(GxEPD_WHITE);
    display.update();
    
    // Connect to WiFi
    connectToWiFi();
    
    // Update display with battery status
    updateDisplay();
}

void loop() {
    // Check WiFi connection status
    if (WiFi.status() != WL_CONNECTED && wifiConnected) {
        Serial.println("WiFi connection lost");
        wifiConnected = false;
        updateDisplay();
    } else if (WiFi.status() == WL_CONNECTED && !wifiConnected) {
        Serial.println("WiFi reconnected");
        wifiConnected = true;
        updateWeather();
        updateDisplay();
    }
    
    // Update weather periodically
    if (wifiConnected && (millis() - lastWeatherUpdate >= weatherUpdateInterval)) {
        Serial.println("Time to update weather");
        updateWeather();
        lastWeatherUpdate = millis();
        updateDisplay();
    }
    
    delay(100); // Small delay to prevent CPU hogging
} 