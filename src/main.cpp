#include <Arduino.h>
#include <GxEPD2_4G_4G.h>
#include <gdey/GxEPD2_213_GDEY0213B74.h>
#include <time.h>
#include "battery.h"
#include "wifi_connection.h"
#include "Weather.h"
#include <Fonts/FreeSans9pt7b.h>  // Smaller font for general text
#include <Fonts/FreeSans12pt7b.h> // Larger font for weather data
#include "boards.h"  // Include for LED pin definition
#include "assets/WifiErrorIcon.h"

// WiFi credentials
const char* ssid = ":(";
const char* password = "20009742591595504581";

// Location coordinates
const float latitude = 52.520008;  // Berlin latitude
const float longitude = 13.404954; // Berlin longitude

GxEPD2_4G_4G<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT> display(GxEPD2_213_GDEY0213B74(/*CS=5*/ SS, /*DC=*/17, /*RST=*/16, /*BUSY=*/4));

const unsigned long sleepTime = 300000000; // Deep sleep time in microseconds (5 minutes)

WiFiConnection wifi(ssid, password);
Weather weather(latitude, longitude);

// Forward declarations
void displayWeather(Weather& weather);
void goToSleep();
void displayWifiErrorIcon();
void drawWindDirectionIndicator(int x, int y, int radius, int direction);

void drawWindDirectionIndicator(int x, int y, int radius, int direction) {
    display.drawCircle(x, y, radius, GxEPD_LIGHTGREY);
    
    // Calculate the end point of the direction line
    // Meteorological wind direction: 0° = wind FROM North, 90° = wind FROM East
    // We need to:
    // 1. Convert from "wind from" to "wind to" by adding 180°
    // 2. Adjust for screen coordinates where Y increases downward
    
    // Convert from "wind from" to "wind to"
    int adjustedDir = (direction + 180) % 360;
    
    // In screen coordinates: 0° is right, clockwise rotation
    // North is upward (270°), East is right (0°), South is down (90°), West is left (180°)
    float angleRadians = adjustedDir * PI / 180.0;
    
    // Calculate end point (note: sin is negated because screen Y increases downward)
    int endX = x + round(radius * sin(angleRadians));
    int endY = y - round(radius * cos(angleRadians));
    
    // Draw the direction line
    display.drawLine(x, y, endX, endY, GxEPD_DARKGREY);
    
    // Draw a small circle at the center for better visibility
    display.fillCircle(x, y, 2, GxEPD_BLACK);
}

void displayWeather(Weather& weather) {
    Serial.println("Updating display...");
    
    display.init(115200);
    display.setRotation(1);
    display.fillScreen(GxEPD_WHITE);

    // Display Battery Status at top right
    display.setFont(&FreeSans9pt7b);
    display.setTextColor(GxEPD_LIGHTGREY);
    String batteryStatus = getBatteryStatus();
    int16_t x1_batt, y1_batt;
    uint16_t w_batt, h_batt;
    display.getTextBounds(batteryStatus, 0, 0, &x1_batt, &y1_batt, &w_batt, &h_batt);
    display.setCursor(display.width() - w_batt - 5, h_batt + 5); // 5px padding from top and right
    display.print(batteryStatus);

    // Display Last Update Time at top left
    display.setFont(&FreeSans9pt7b); // Ensure correct font
    display.setTextColor(GxEPD_LIGHTGREY); // Ensure correct color
    String lastUpdateTime = weather.getLastUpdateTime();
    int16_t x1_time, y1_time;
    uint16_t w_time, h_time;
    display.getTextBounds(lastUpdateTime, 0, 0, &x1_time, &y1_time, &w_time, &h_time);
    display.setCursor(5, h_time + 5); // 5px padding from top and left
    display.print(lastUpdateTime);
    
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeSans12pt7b);

    display.setCursor(10, 100); 
    display.println(weather.getWeatherText());
    
    display.setCursor(10, 125);
    display.println(weather.getWindText());
    
    // Draw wind direction indicator
    // Place it on the right-most side of the display aligned with the weather data
    int radius = 25; // Circle radius
    int padding = 10; // Padding from the edge
    int windDirX = display.width() - radius - padding; // Right side with padding
    int windDirY = 100; // Between weather and wind text (vertically aligned with weather data)
    drawWindDirectionIndicator(windDirX, windDirY, radius, weather.getWindDirection());
    
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
    
    weather.update();
    displayWeather(weather);
    
    goToSleep();
}

void loop() {
    // Most of the work is done in setup() followed by deep sleep
    // The loop() function won't be executed unless something prevents deep sleep
} 