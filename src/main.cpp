#include <Arduino.h>
#include <GxEPD2_4G_4G.h>
#include <gdey/GxEPD2_213_GDEY0213B74.h>
#include <time.h>

#include "AIWeatherPrompt.h"
#include "ChatGPTClient.h"
#include "ConfigurationScreen.h"
#include "ConfigurationServer.h"
#include "MessageScreen.h"
#include "MeteogramWeatherScreen.h"
#include "Weather.h"
#include "WiFiConnection.h"
#include "WifiErrorScreen.h"
#include "battery.h"
#include "boards.h"

RTC_DATA_ATTR char wifiSSID[64] = ":(";
RTC_DATA_ATTR char wifiPassword[64] = "20009742591595504581";

// Berlin
const float latitude = 52.520008;
const float longitude = 13.404954;

GxEPD2_4G_4G<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT> display(
    GxEPD2_213_GDEY0213B74(/*CS=5*/ SS, /*DC=*/17, /*RST=*/16, /*BUSY=*/4));

const unsigned long deepSleepMicros = 900000000;  // Deep sleep time in microseconds (15 minutes)

Weather weather(latitude, longitude);
MeteogramWeatherScreen weatherScreen(display, weather);
WifiErrorScreen errorScreen(display);
MessageScreen messageScreen(display);
ChatGPTClient chatGPTClient;
AIWeatherPrompt weatherPrompt;
ConfigurationServer configurationServer;

enum ScreenType { CONFIG_SCREEN = 0, METEOGRAM_SCREEN = 1, MESSAGE_SCREEN = 2, SCREEN_COUNT = 3 };

RTC_DATA_ATTR int currentScreenIndex = METEOGRAM_SCREEN;

void goToSleep(uint64_t sleepTime);
void checkWakeupReason();
void displayCurrentScreen();
void cycleToNextScreen();
bool isButtonWakeup();
void runConfigurationMode();
void updateWiFiCredentials(const String& newSSID, const String& newPassword);
bool hasValidWiFiCredentials();

bool hasValidWiFiCredentials() { return strlen(wifiSSID) > 0 && strlen(wifiPassword) > 0; }

bool isButtonWakeup() {
  esp_sleep_wakeup_cause_t wakeupReason = esp_sleep_get_wakeup_cause();
  return (wakeupReason == ESP_SLEEP_WAKEUP_EXT0);
}

void cycleToNextScreen() {
  currentScreenIndex = (currentScreenIndex + 1) % SCREEN_COUNT;
  Serial.println("Cycled to screen: " + String(currentScreenIndex));
}

void displayCurrentScreen() {
  switch (currentScreenIndex) {
    case CONFIG_SCREEN: {
      Serial.println("Displaying configuration screen");
      ConfigurationScreen configurationScreen(display, configurationServer.getWifiAccessPointName(),
                                              configurationServer.getWifiAccessPointPassword());
      configurationScreen.render();

      configurationServer.run([](const String& ssid, const String& password) {
        updateWiFiCredentials(ssid, password);
        configurationServer.stop();
      });

      while (configurationServer.isRunning()) {
        configurationServer.handleRequests();

        if (digitalRead(BUTTON_1) == LOW) {
          delay(50);
          if (digitalRead(BUTTON_1) == LOW) {
            Serial.println("Button pressed - exiting configuration mode");
            break;
          }
        }

        delay(10);
      }

      configurationServer.stop();
      break;
    }
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

void updateWiFiCredentials(const String& newSSID, const String& newPassword) {
  if (newSSID.length() >= sizeof(wifiSSID)) {
    Serial.println("Error: SSID too long, maximum length is " + String(sizeof(wifiSSID) - 1));
    return;
  }

  if (newPassword.length() >= sizeof(wifiPassword)) {
    Serial.println("Error: Password too long, maximum length is " + String(sizeof(wifiPassword) - 1));
    return;
  }

  memset(wifiSSID, 0, sizeof(wifiSSID));
  memset(wifiPassword, 0, sizeof(wifiPassword));

  strncpy(wifiSSID, newSSID.c_str(), sizeof(wifiSSID) - 1);
  strncpy(wifiPassword, newPassword.c_str(), sizeof(wifiPassword) - 1);

  Serial.println("WiFi credentials updated in RTC memory");
}

void runConfigurationMode() {
  Serial.println("Entering configuration mode...");

  ConfigurationScreen configurationScreen(display, configurationServer.getWifiAccessPointName(),
                                          configurationServer.getWifiAccessPointPassword());
  configurationScreen.render();

  configurationServer.run([](const String& ssid, const String& password) {
    updateWiFiCredentials(ssid, password);
    configurationServer.stop();
  });

  unsigned long configStartTime = millis();

  while (configurationServer.isRunning()) {
    configurationServer.handleRequests();

    if (digitalRead(BUTTON_1) == LOW) {
      delay(50);  // Debounce
      if (digitalRead(BUTTON_1) == LOW) {
        Serial.println("Button pressed - exiting configuration mode");
        break;
      }
    }

    delay(10);  // Small delay to prevent watchdog issues
  }

  configurationServer.stop();
  Serial.println("Configuration mode ended");
  //   ESP.restart();
}

void goToSleep(uint64_t sleepTime) {
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

  if (!isButtonWakeup()) {
    if (!hasValidWiFiCredentials()) {
      currentScreenIndex = CONFIG_SCREEN;
    } else {
      currentScreenIndex = METEOGRAM_SCREEN;
    }
  }

  if (isButtonWakeup()) {
    cycleToNextScreen();
  }

  if (currentScreenIndex != CONFIG_SCREEN) {
    WiFiConnection wifi(wifiSSID, wifiPassword);
    wifi.connect();
    if (!wifi.isConnected()) {
      Serial.println("Failed to connect to WiFi");
      errorScreen.render();
      goToSleep(deepSleepMicros);
      return;
    }
  }

  displayCurrentScreen();

  if (currentScreenIndex == CONFIG_SCREEN) {
    goToSleep(1000000);  // 1 second
  } else {
    goToSleep(deepSleepMicros);
  }
}

void loop() {}