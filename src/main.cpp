#include <Arduino.h>
#include <GxEPD2_4G_4G.h>
#include <gdey/GxEPD2_213_GDEY0213B74.h>
#include <time.h>
#include "battery.h"
#include "WiFiConnection.h"
#include "Weather.h"
#include "MeteogramWeatherScreen.h"
#include "WifiErrorScreen.h"
#include "MessageScreen.h"
#include "GeminiClient.h"
#include "AIWeatherPrompt.h"
#include "boards.h"

RTC_DATA_ATTR int currentScreenIndex = 0;

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
MessageScreen messageScreen(display);
GeminiClient geminiClient;
AIWeatherPrompt weatherPrompt;

enum ScreenType {
    METEOGRAM_SCREEN = 0,
    MESSAGE_SCREEN = 1,
    SCREEN_COUNT = 2
};

void goToSleep();
void checkWakeupReason();
void displayCurrentScreen();
void cycleToNextScreen();
bool isButtonWakeup();

bool isButtonWakeup() {
    esp_sleep_wakeup_cause_t wakeupReason = esp_sleep_get_wakeup_cause();
    return (wakeupReason == ESP_SLEEP_WAKEUP_EXT0);
}

void cycleToNextScreen() {
    currentScreenIndex = (currentScreenIndex + 1) % SCREEN_COUNT;
    Serial.println("Cycled to screen: " + String(currentScreenIndex));
}

void displayCurrentScreen() {
    switch(currentScreenIndex) {
        case METEOGRAM_SCREEN:
            Serial.println("Displaying meteogram screen");
            weather.update();
            weatherScreen.render();
            break;
        case MESSAGE_SCREEN: {
            Serial.println("Displaying message screen");
            weather.update();
            String prompt = weatherPrompt.generatePrompt(weather);
            String geminiResponse = geminiClient.generateContent(prompt);
            Serial.println("Gemini Response: " + geminiResponse);
            
            messageScreen.setMessageText(geminiResponse);
            messageScreen.render();
            break;
        }
        default:
            Serial.println("Unknown screen index, defaulting to meteogram");
            currentScreenIndex = METEOGRAM_SCREEN;
            weather.update();
            weatherScreen.render();
            break;
    }
}

void goToSleep() {
    Serial.println("Going to deep sleep for " + String(sleepTime / 1000000) + " seconds");
    Serial.println("Press button to wake up early and cycle screens");
    
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_39, 0);
    esp_sleep_enable_timer_wakeup(sleepTime);
    esp_deep_sleep_start();
}

void setup() {
    Serial.begin(115200);
    
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
    
    if (isButtonWakeup()) {
        cycleToNextScreen();
    }
    
    displayCurrentScreen();
    goToSleep();
}

void loop() {
} 