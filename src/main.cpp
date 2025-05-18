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

void displayWeather(Weather& weather) {
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
    display.println(weather.getLastUpdateTime());
    
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeSans12pt7b);

    display.setCursor(10, 100); 
    display.println(weather.getWeatherText());
    
    display.setCursor(10, 125);
    display.println(weather.getWindText());
    
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