#include <Arduino.h>
#include <GxEPD2_4G_4G.h>
#include <gdey/GxEPD2_213_GDEY0213B74.h>
#include <time.h>
#include "battery.h"
#include "WiFiConnection.h"
#include "Weather.h"
#include "Rendering.h"
#include "WifiErrorScreen.h"
#include "GeminiClient.h"
#include "boards.h"

const char* ssid = ":(";
const char* password = "20009742591595504581";

// Berlin
const float latitude = 52.520008;
const float longitude = 13.404954;

GxEPD2_4G_4G<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT> display(GxEPD2_213_GDEY0213B74(/*CS=5*/ SS, /*DC=*/17, /*RST=*/16, /*BUSY=*/4));

const unsigned long sleepTime = 900000000; // Deep sleep time in microseconds (15 minutes)

WiFiConnection wifi(ssid, password);
Weather weather(latitude, longitude);
MeteogramWeatherScreen weatherScreen(display, weather);
WifiErrorScreen errorScreen(display);
GeminiClient geminiClient;

void goToSleep();
void checkWakeupReason();

void checkWakeupReason() {
    esp_sleep_wakeup_cause_t wakeupReason = esp_sleep_get_wakeup_cause();
    
    switch(wakeupReason) {
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("Wakeup caused by external signal using RTC_IO");
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("Wakeup caused by timer");
            break;
        default:
            Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeupReason);
            break;
    }
}

void goToSleep() {
    Serial.println("Going to deep sleep for " + String(sleepTime / 1000000) + " seconds");
    Serial.println("Press button to wake up early");
    
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_39, 0);
    esp_sleep_enable_timer_wakeup(sleepTime);
    esp_deep_sleep_start();
}

void setup() {
    Serial.begin(115200);
    
    checkWakeupReason();
    
    pinMode(BATTERY_PIN, INPUT);
    pinMode(BUTTON_1, INPUT_PULLUP);
    SPI.begin(18, 19, 23);
    
    wifi.connect();
    if (!wifi.isConnected()) {
        Serial.println("Failed to connect to WiFi");
        errorScreen.render();
        goToSleep();
        return;
    }
    
    // Initialize Gemini client
    geminiClient.initialize();
    
    // Example usage of Gemini API
    String prompt = "Generate a brief weather-related motivational quote for today.";
    String geminiResponse = geminiClient.generateContent(prompt);
    Serial.println("Gemini Response: " + geminiResponse);
    
    weather.update();
    weatherScreen.render();
    
    goToSleep();
}

void loop() {
} 