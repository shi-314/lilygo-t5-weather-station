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
#include "ChatGPTClient.h"
#include "AIWeatherPrompt.h"
#include "boards.h"
#include "ConfigurationServer.h"

const char* ssid = ":(";
const char* password = "20009742591595504581";

// Berlin
const float latitude = 52.520008;
const float longitude = 13.404954;

GxEPD2_4G_4G<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT> display(GxEPD2_213_GDEY0213B74(/*CS=5*/ SS, /*DC=*/17, /*RST=*/16, /*BUSY=*/4));

const unsigned long sleepTime = 900000000; // Deep sleep time in microseconds (15 minutes)
const unsigned long configModeTimeout = 300000; // 5 minutes in config mode before restart

WiFiConnection wifi(ssid, password);
Weather weather(latitude, longitude);
MeteogramWeatherScreen weatherScreen(display, weather);
WifiErrorScreen errorScreen(display);
MessageScreen messageScreen(display);
ChatGPTClient chatGPTClient;
AIWeatherPrompt weatherPrompt;
ConfigurationServer configurationServer;

enum ScreenType {
    METEOGRAM_SCREEN = 0,
    MESSAGE_SCREEN = 1,
    SCREEN_COUNT = 2
};

RTC_DATA_ATTR int currentScreenIndex = METEOGRAM_SCREEN;
RTC_DATA_ATTR bool enterConfigMode = false;

void goToSleep();
void checkWakeupReason();
void displayCurrentScreen();
void cycleToNextScreen();
bool isButtonWakeup();
void runConfigurationMode();

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
            String chatGPTResponse = chatGPTClient.generateContent(prompt);
            Serial.println("ChatGPT Response: " + chatGPTResponse);
            
            messageScreen.setMessageText(chatGPTResponse);
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

void runConfigurationMode() {
    Serial.println("Entering configuration mode...");
    
    // Show configuration message on display
    display.init(115200);
    display.setRotation(1);
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setFont();
    display.setCursor(10, 30);
    display.println("Configuration Mode");
    display.setCursor(10, 50);
    display.println("Connect to WiFi:");
    display.setCursor(10, 70);
    display.println("WeatherStation-Config");
    display.setCursor(10, 90);
    display.println("Password: configure123");
    display.displayWindow(0, 0, display.width(), display.height());
    display.hibernate();
    
    configurationServer.run();
    
    unsigned long configStartTime = millis();
    
    // Keep the configuration server running until timeout or configuration complete
    while (millis() - configStartTime < configModeTimeout) {
        configurationServer.handleRequests();
        
        // Check if button is pressed to exit config mode
        if (digitalRead(BUTTON_1) == LOW) {
            delay(50); // Debounce
            if (digitalRead(BUTTON_1) == LOW) {
                Serial.println("Button pressed - exiting configuration mode");
                break;
            }
        }
        
        delay(10); // Small delay to prevent watchdog issues
    }
    
    configurationServer.stop();
    Serial.println("Configuration mode ended");
    
    // Restart the device to apply new configuration
    ESP.restart();
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

    if (enterConfigMode) {
        enterConfigMode = false;
        runConfigurationMode();
        return;
    }
    
    wifi.connect();
    if (!wifi.isConnected()) {
        Serial.println("Failed to connect to WiFi");
        errorScreen.render();
        goToSleep();
        return;
    }
    
    if (isButtonWakeup()) {
        unsigned long buttonPressStart = millis();
        bool buttonHeld = true;
        
        while (millis() - buttonPressStart < 3000) {
            if (digitalRead(BUTTON_1) == HIGH) {
                buttonHeld = false;
                break;
            }
            delay(10);
        }
        
        if (buttonHeld) {
            Serial.println("Button held for 3 seconds - entering configuration mode");
            runConfigurationMode();
            return;
        } else {
            cycleToNextScreen();
        }
    }
    
    displayCurrentScreen();
    goToSleep();
}

void loop() {
} 